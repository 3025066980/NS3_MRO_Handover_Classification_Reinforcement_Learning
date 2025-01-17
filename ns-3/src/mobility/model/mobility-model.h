/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef MOBILITY_MODEL_H
#define MOBILITY_MODEL_H

#include "ns3/vector.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"

namespace ns3 {

/**
 * \ingroup mobility
 * \brief Keep track of the current position and velocity of an object.
 *
 * All space coordinates in this class and its subclasses are
 * understood to be meters or meters/s. i.e., they are all
 * metric international units.
 *
 * This is a base class for all specific mobility models.
 */
class MobilityModel : public Object
{
public:
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  MobilityModel ();
  virtual ~MobilityModel () = 0;

  /**
   * \return the current position
   */
  Vector GetPosition (void) const;
  /**
   * \param position the position to set.
   */
  void SetPosition (const Vector &position);
  /**
   * \return the current velocity.
   */
  Vector GetVelocity (void) const;
  /**
   * \param position a reference to another mobility model
   * \return the distance between the two objects. Unit is meters.
   */
  double GetDistanceFrom (Ptr<const MobilityModel> position) const;

  double GetAngleFrom(Ptr<const MobilityModel> position) const;//THANG:For Manhattan Mobility Model
  /**
   * \param other reference to another object's mobility model
   * \return the relative speed between the two objects. Unit is meters/s.
   */
  double GetRelativeSpeed (Ptr<const MobilityModel> other) const;
  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

  /**
   *  TracedCallback signature.
   *
   * \param [in] model Value of the MobilityModel.
   * @{
   */
  typedef void (* TracedCallback)(const Ptr<const MobilityModel> model);
  
  /**
   * TracedCallback signature for course change notifications.
   *
   * If the callback is connected using Config::ConnectWithoutContext()
   * omit the \c context argument from the signature.
   *
   * \param [in] context The context string, supplied by the Trace source.
   * \param [in] model The MobilityModel which is changing course.
   */
  typedef void (* CourseChangeTracedCallback)
    (const std::string context, const Ptr<const MobilityModel> model);
  
  /*
   * Added by THANG
   */
  uint64_t GetNodeId();//return IMSI or cellId
  bool GetNodeType();//return UE or eNB

protected:
  /**
   * Must be invoked by subclasses when the course of the
   * position changes to notify course change listeners.
   */
  void NotifyCourseChange (void) const;
private:
  /**
   * \return the current position.
   *
   * Concrete subclasses of this base class must 
   * implement this method.
   */
  virtual Vector DoGetPosition (void) const = 0;
  /**
   * \param position the position to set.
   *
   * Concrete subclasses of this base class must 
   * implement this method.
   */
  virtual void DoSetPosition (const Vector &position) = 0;
  /**
   * \return the current velocity.
   *
   * Concrete subclasses of this base class must 
   * implement this method.
   */
  virtual Vector DoGetVelocity (void) const = 0;
  /**
   * The default implementation does nothing but return the passed-in
   * parameter.  Subclasses using random variables are expected to
   * override this.
   * \param start  starting stream index
   * \return the number of streams used
   */
  virtual int64_t DoAssignStreams (int64_t start);

  /**
   * Used to alert subscribers that a change in direction, velocity,
   * or position has occurred.
   */
  ns3::TracedCallback<Ptr<const MobilityModel> > m_courseChangeTrace;

  /*
   * THANG add for discriminate eNB and UE node
   */
  uint64_t m_nodeId;//if m_isUE = 1, this is IMSI; if n_isUE = 0, this is cellId
  bool m_isUE; //1: UE Node; 0: eNB node

};

} // namespace ns3

#endif /* MOBILITY_MODEL_H */
