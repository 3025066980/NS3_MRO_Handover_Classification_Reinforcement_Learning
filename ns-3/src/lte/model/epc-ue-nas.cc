/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include <ns3/fatal-error.h>
#include <ns3/log.h>

#include <ns3/epc-helper.h>

#include "lte-enb-net-device.h"
#include "epc-ue-nas.h"
#include "lte-as-sap.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcUeNas");



/// Map each of UE NAS states to its string representation.
static const std::string g_ueNasStateName[EpcUeNas::NUM_STATES] =
{
  "OFF",
  "ATTACHING",
  "IDLE_REGISTERED",
  "CONNECTING_TO_EPC",
  "ACTIVE"
};

/**
 * \param s The UE NAS state.
 * \return The string representation of the given state.
 */
static inline const std::string & ToString (EpcUeNas::State s)
{
  return g_ueNasStateName[s];
}




NS_OBJECT_ENSURE_REGISTERED (EpcUeNas);

EpcUeNas::EpcUeNas ()
  : m_state (OFF),
    m_csgId (0),
    m_asSapProvider (0),
    m_bidCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_asSapUser = new MemberLteAsSapUser<EpcUeNas> (this);
}


EpcUeNas::~EpcUeNas ()
{
  NS_LOG_FUNCTION (this);
}

void
EpcUeNas::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_asSapUser;
}

TypeId
EpcUeNas::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcUeNas")
    .SetParent<Object> ()
    .SetGroupName("Lte")
    .AddConstructor<EpcUeNas> ()
    .AddTraceSource ("StateTransition",
                     "fired upon every UE NAS state transition",
                     MakeTraceSourceAccessor (&EpcUeNas::m_stateTransitionCallback),
                     "ns3::EpcUeNas::StateTracedCallback")
  ;
  return tid;
}

void 
EpcUeNas::SetDevice (Ptr<NetDevice> dev)
{
  NS_LOG_FUNCTION (this << dev);
  m_device = dev;
}

void 
EpcUeNas::SetImsi (uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi);
  m_imsi = imsi;
}

void
EpcUeNas::SetCsgId (uint32_t csgId)
{
  NS_LOG_FUNCTION (this << csgId);
  m_csgId = csgId;
  m_asSapProvider->SetCsgWhiteList (csgId);
}

uint32_t
EpcUeNas::GetCsgId () const
{
  NS_LOG_FUNCTION (this);
  return m_csgId;
}

void
EpcUeNas::SetAsSapProvider (LteAsSapProvider* s)
{
  NS_LOG_FUNCTION (this << s);
  m_asSapProvider = s;
}

LteAsSapUser*
EpcUeNas::GetAsSapUser ()
{
  NS_LOG_FUNCTION (this);
  return m_asSapUser;
}

void
EpcUeNas::SetForwardUpCallback (Callback <void, Ptr<Packet> > cb)
{
  NS_LOG_FUNCTION (this);
  m_forwardUpCallback = cb;
}

void
EpcUeNas::StartCellSelection (uint16_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << dlEarfcn);
  m_asSapProvider->StartCellSelection (dlEarfcn);
}

void 
EpcUeNas::Connect ()
{
  NS_LOG_FUNCTION (this);

  // tell RRC to go into connected mode
  m_asSapProvider->Connect ();
}

void
EpcUeNas::Connect (uint16_t cellId, uint16_t dlEarfcn)
{
  NS_LOG_FUNCTION (this << cellId << dlEarfcn);

  // force the UE RRC to be camped on a specific eNB
  m_asSapProvider->ForceCampedOnEnb (cellId, dlEarfcn);

  // tell RRC to go into connected mode
  m_asSapProvider->Connect ();
}


void 
EpcUeNas::Disconnect ()
{
  NS_LOG_FUNCTION (this);
  m_asSapProvider->Disconnect ();
  SwitchToState (OFF);
}


void 
EpcUeNas::ActivateEpsBearer (EpsBearer bearer, Ptr<EpcTft> tft)
{
  NS_LOG_FUNCTION (this);
  switch (m_state)
    {
    case ACTIVE:
      NS_FATAL_ERROR ("the necessary NAS signaling to activate a bearer after the initial context has already been setup is not implemented");
      break;

    default:
      BearerToBeActivated btba;
      btba.bearer = bearer;
      btba.tft = tft;
      m_bearersToBeActivatedList.push_back (btba);
      break;
    }
}

bool
EpcUeNas::Send (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  switch (m_state)
    {
    case ACTIVE:
      {
        uint32_t id = m_tftClassifier.Classify (packet, EpcTft::UPLINK);
        NS_ASSERT ((id & 0xFFFFFF00) == 0);
        uint8_t bid = (uint8_t) (id & 0x000000FF);
        if (bid == 0)
          {
            return false;
          }
        else
          {
            m_asSapProvider->SendData (packet, bid); 
            return true;
          }
      }
      break;

    default:
      NS_LOG_WARN (this << " NAS OFF, discarding packet");
      return false;
      break;
    }
}

void 
EpcUeNas::DoNotifyConnectionSuccessful ()
{
  NS_LOG_FUNCTION (this);
//  NS_LOG_UNCOND("EpcUeNas::DoNotifyConnectionSuccessful () ");
  SwitchToState (ACTIVE); // will eventually activate dedicated bearers
}

void
EpcUeNas::DoNotifyConnectionFailed ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_UNCOND("EpcUeNas::DoNotifyConnectionFailed () ");
  // immediately retry the connection
  Simulator::ScheduleNow (&LteAsSapProvider::Connect, m_asSapProvider);
}

void
EpcUeNas::DoRecvData (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  m_forwardUpCallback (packet);
}

void 
EpcUeNas::DoNotifyConnectionReleased ()
{
  NS_LOG_FUNCTION (this);
  SwitchToState (OFF);
}

void 
EpcUeNas::DoActivateEpsBearer (EpsBearer bearer, Ptr<EpcTft> tft)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_bidCounter < 11, "cannot have more than 11 EPS bearers");
  uint8_t bid = ++m_bidCounter;
  NS_LOG_DEBUG ("EpcUeNas::DoActivateEpsBearer active bearer id " << (uint32_t)bid);
  m_tftClassifier.Add (tft, bid);
}

EpcUeNas::State
EpcUeNas::GetState () const
{
  NS_LOG_FUNCTION (this);
  return m_state;
}

void 
EpcUeNas::SwitchToState (State newState)
{
  NS_LOG_FUNCTION (this << ToString (newState));
  State oldState = m_state;
  m_state = newState;
  NS_LOG_INFO ("IMSI " << m_imsi << " NAS " << ToString (oldState) << " --> " << ToString (newState));
  m_stateTransitionCallback (oldState, newState);

  // actions to be done when entering a new state:
  switch (m_state)
    {
    case ACTIVE:
    NS_LOG_DEBUG ("EpcUeNas::SwitchToState activelist has " << m_bearersToBeActivatedList.size());
      for (std::list<BearerToBeActivated>::iterator it = m_bearersToBeActivatedList.begin ();
           it != m_bearersToBeActivatedList.end ();
           m_bearersToBeActivatedList.erase (it++))//active only one first time
        {
          DoActivateEpsBearer (it->bearer, it->tft);
        }
      break;

    default:
    	NS_LOG_DEBUG ("EpcUeNas::SwitchToState " << ToString (oldState) << " --> " << ToString (newState));
      break;
    }

}


} // namespace ns3

