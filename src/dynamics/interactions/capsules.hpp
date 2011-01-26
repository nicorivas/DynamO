/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2010  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CICapsules_H
#define CICapsules_H

#include "captures.hpp"

class ICapsules: public ISingleCapture
{
public:
  ICapsules(DYNAMO::SimData*, double, double, double, C2Range*);

  ICapsules(const XMLNode&, DYNAMO::SimData*);

  void operator<<(const XMLNode&);

  virtual double getInternalEnergy() const { return 0.0; }

  virtual void initialise(size_t);

  virtual double maxIntDist() const;

  virtual double hardCoreDiam() const;

  virtual void rescaleLengths(double);

  virtual Interaction* Clone() const;
  
  virtual IntEvent getEvent(const Particle&, const Particle&) const;
 
  virtual void runEvent(const Particle&, const Particle&, const IntEvent&) const;
   
  virtual void outputXML(xml::XmlStream&) const;

  virtual void checkOverlaps(const Particle&, const Particle&) const;
 
  virtual bool captureTest(const Particle&, const Particle&) const;

  virtual void write_povray_desc(const DYNAMO::RGB&, 
				 const size_t&, std::ostream&) const;

protected:
  double length;
  double l2;
  double e;
  double r;
};

#endif