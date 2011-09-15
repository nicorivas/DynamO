/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#pragma once
#include <coil/RenderObj/DataSet.hpp>

namespace coil {
  class AttributeSelector: public Gtk::VBox
  {
  public:
    AttributeSelector(size_t attrnum, bool enableDataFiltering = true);
    
    void buildEntries(std::string name, DataSet& ds, size_t minComponents, size_t maxComponents, 
		      int typeMask, size_t components, int defaultMask = 0)
    {
      _components = components;
      _label.set_text(name);
      _model->clear();
      
      updateGui();

      if (_components)
	{
	  Gtk::TreeModel::Row row = *(_model->append());
	  row[_modelColumns.m_name] = "Single Value";
	}

      for (DataSet::iterator iPtr = ds.begin();
	   iPtr != ds.end(); ++iPtr)
	if (((iPtr->second->getType()) & typeMask)
	    && (iPtr->second->components() >=  minComponents)
	    && (iPtr->second->components() <=  maxComponents))
	  {
	    Gtk::TreeModel::Row row = *(_model->append());
	    row[_modelColumns.m_name] = iPtr->first;
	    row[_modelColumns.m_ptr] = iPtr->second;
	  }
      
      typedef Gtk::TreeModel::Children::iterator iterator;
      iterator selected = _comboBox.get_model()->children().begin();

      for (iterator iPtr = _comboBox.get_model()->children().begin();
	   iPtr != _comboBox.get_model()->children().end(); ++iPtr)
	{
	  std::tr1::shared_ptr<Attribute> attr_ptr = (*iPtr)[_modelColumns.m_ptr];
	  if ((attr_ptr) && (attr_ptr->getType() & defaultMask))
	    {
	      selected = iPtr;
	      break;
	    }
	}

      _comboBox.set_active(selected);
    }
    
    struct ModelColumns : Gtk::TreeModelColumnRecord
    {
      ModelColumns()
      { add(m_name); add(m_ptr); }
      
      Gtk::TreeModelColumn<Glib::ustring> m_name;
      Gtk::TreeModelColumn<std::tr1::shared_ptr<Attribute> > m_ptr;
    };

    virtual void bindAttribute()
    {
      Gtk::TreeModel::iterator iter = _comboBox.get_active();

      if (singleValueMode())
	setConstantAttribute(_attrnum);
      else
	{
	  std::tr1::shared_ptr<Attribute> ptr = (*iter)[_modelColumns.m_ptr];
	  //We have an attribute, check the mode the ComboBox is in,
	  //and determine if we have to do something with the data!

	  //Detect if it is in simple pass-through mode
	  if ((!_componentSelect.get_visible())
	      || (_componentSelect.get_active_row_number() == 0))
	    {
	      ptr->bindAttribute(_attrnum, false);
	      return;
	    }
	    
	  //Check if the data actually needs updating
	  if ((_lastAttribute != ptr.get())
	      || (_lastAttributeDataCount != ptr->getUpdateCount())
	      || (_lastComponentSelected != _componentSelect.get_active_row_number())
	      || _filteredData.empty())
	    {
	      _lastAttribute = ptr.get();
	      _lastAttributeDataCount = ptr->getUpdateCount();
	      _lastComponentSelected = _componentSelect.get_active_row_number();
	      
	      std::vector<GLfloat> scalardata;
	      generateFilteredData(scalardata, ptr, _lastComponentSelected);
	      _filteredData = scalardata;
	    }

	  _filteredData.attachToAttribute(_attrnum, 1, 1);
	}
    }

    virtual std::vector<GLfloat> getValue(size_t id)
    {
      Gtk::TreeModel::iterator iter = _comboBox.get_active();
      if (!iter) return std::vector<GLfloat>();

      std::tr1::shared_ptr<Attribute> ptr = (*iter)[_modelColumns.m_ptr];
      if (!ptr) return std::vector<GLfloat>();
      
      std::vector<GLfloat> retval(ptr->components());

      for (size_t i(0); i < ptr->components(); ++i)
	retval[i] = (*ptr)[id * ptr->components() + i];
      
      return retval;
    }

    ModelColumns _modelColumns;
    Gtk::ComboBox _comboBox;
    Gtk::ComboBoxText _componentSelect;
    Gtk::Label _label;
    Gtk::Label _singleValueLabel;
    Glib::RefPtr<Gtk::ListStore> _model;
    Gtk::Entry _scalarvalues[4];
    Gtk::HBox _selectorRow;

  protected:
    Attribute* _lastAttribute;
    size_t _lastAttributeDataCount;
    int _lastComponentSelected;
    magnet::GL::Buffer<GLfloat> _filteredData;

    magnet::GL::Context* _context;
        
    size_t _components;

    size_t _attrnum;
    bool _enableDataFiltering;
    
    inline bool singleValueMode()
    {
      Gtk::TreeModel::iterator iter = _comboBox.get_active();
      if (!iter) return true;
      std::tr1::shared_ptr<Attribute> ptr = (*iter)[_modelColumns.m_ptr];
      return !ptr;
    }

    inline void generateFilteredData(std::vector<GLfloat>& scalardata,
				     const std::tr1::shared_ptr<Attribute>& ptr,
				     size_t mode)
    {
      //Update the data according to what was selected
      scalardata.resize(ptr->num_elements());
      const size_t components = ptr->components();
      const std::vector<GLfloat>& attrdata = *ptr;
      
      if (mode == 1)
	//Magnitude calculation
	{
	  for (size_t i(0); i < scalardata.size(); ++i)
	    {
	      scalardata[i] = 0;
	      for (size_t j(0); j < components; ++j)
		{
		  GLfloat val = attrdata[i * components + j];
		  scalardata[i] += val * val;
		}
	      scalardata[i] = std::sqrt(scalardata[i]);
	    }
	}
      else
	{
	  //Component wise selection
	  size_t component = mode - 2;
#ifdef COIL_DEBUG
	  if (component >= components)
	    M_throw() << "Trying to filter an invalid component";
#endif
	  for (size_t i(0); i < scalardata.size(); ++i)
	    scalardata[i] = attrdata[i * components + component];
	}
    }

    inline void setConstantAttribute(size_t attr)
    {
      _context->disableAttributeArray(attr);

      GLfloat val[4] = {1,1,1,1};

      for (size_t i(0); i < 4; ++i)
	try {
	  val[i] = boost::lexical_cast<double>(_scalarvalues[i].get_text());
	} catch (...) {}
      
      _context->setAttribute(attr, val[0], val[1], val[2], val[3]);
    }

    inline virtual void updateGui()
    {
      _singleValueLabel.set_visible(false);
      for (size_t i(0); i < 4; ++i)
	_scalarvalues[i].hide();

      bool singlevalmode = singleValueMode();

      if (_components && singlevalmode)
	{
	  _singleValueLabel.set_visible(true);
	  for (size_t i(0); i < _components; ++i)
	    _scalarvalues[i].show();
	}

      _componentSelect.clear_items();
      if (singlevalmode || !_enableDataFiltering)
	_componentSelect.set_visible(false);
      else
	{
	  _componentSelect.set_visible(true);

	  Gtk::TreeModel::iterator iter = _comboBox.get_active();
	  std::tr1::shared_ptr<Attribute> ptr = (*iter)[_modelColumns.m_ptr];

	  _componentSelect.append_text("Raw Data");
	  _componentSelect.append_text("Magnitude");
	  _componentSelect.append_text("X");
	    
	  if (ptr->components() > 1)
	    _componentSelect.append_text("Y");
	  if (ptr->components() > 2)
	    _componentSelect.append_text("Z");
	  if (ptr->components() > 3)
	    _componentSelect.append_text("W");
	  
	  //Default to coloring using the magnitude
	  _componentSelect.set_active(1);
	}

      for (size_t i(0); i < _components; ++i)
	_scalarvalues[i].set_sensitive(singlevalmode);
    }
  };


  class AttributeColorSelector : public AttributeSelector
  {
  public:
    AttributeColorSelector();
    
    virtual void bindAttribute()
    {
      Gtk::TreeModel::iterator iter = _comboBox.get_active();

      if (singleValueMode())
	setConstantAttribute(_attrnum);
      else
	{
	  std::tr1::shared_ptr<Attribute> ptr = (*iter)[_modelColumns.m_ptr];
	  //We have an attribute, check the mode the ComboBox is in,
	  //and determine if we have to do something with the data!

	  //Detect if it is in simple pass-through mode
	  if ((_componentSelect.get_visible())
	      && (_componentSelect.get_active_row_number() == 0))
	    {
	      ptr->bindAttribute(_attrnum, false);
	      return;
	    }
	    
	  //Check if the data actually needs updating
	  if ((_lastAttribute != ptr.get())
	      || (_lastAttributeDataCount != ptr->getUpdateCount())
	      || (_lastComponentSelected != _componentSelect.get_active_row_number())
	      || (_lastColorMap != _colorMapSelector.getMode())
	      || _filteredData.empty())
	    {
	      std::vector<GLfloat> scalardata;
	      generateFilteredData(scalardata, ptr, _lastComponentSelected);
	      
	      if (_autoScaling.get_active() && !scalardata.empty())
		{
		  GLfloat min(scalardata[0]), max(scalardata[0]);
		  
		  for (std::vector<GLfloat>::const_iterator iPtr = scalardata.begin();
		       iPtr != scalardata.end();
		       ++iPtr)
		    {
		      min = std::min(min, *iPtr);
		      max = std::max(max, *iPtr);
		      _colorMapSelector.setRange(min, max);
		    }
		}

	      //Now convert to HSV or whatever
	      _filteredData.init(4 * scalardata.size());
	      GLfloat* data_ptr = _filteredData.map();
	      for (size_t i(0); i < scalardata.size(); ++i)
		_colorMapSelector.map(data_ptr + 4 * i, scalardata[i]);
	      
	      _filteredData.unmap();
	      
	      _lastAttribute = ptr.get();
	      _lastAttributeDataCount = ptr->getUpdateCount();
	      _lastComponentSelected = _componentSelect.get_active_row_number();
	      _lastColorMap = _colorMapSelector.getMode();
	    }

	  _filteredData.attachToAttribute(_attrnum, 4, 1);
	}
    }

  protected:
    void colorMapChanged() { _lastColorMap = -2; }

    inline virtual void updateGui()
    {
      AttributeSelector::updateGui();
      if (singleValueMode())
	{
	  _colorMapSelector.hide();
	  _autoScaling.hide();
	}
      else
	{
	  _colorMapSelector.show();
	  _autoScaling.show();
	}
    }

    magnet::gtk::ColorMapSelector _colorMapSelector;
    Gtk::CheckButton _autoScaling;
    int _lastColorMap;
  };


  class AttributeOrientationSelector : public AttributeSelector
  {
  public:
    AttributeOrientationSelector():
      AttributeSelector(magnet::GL::Context::instanceOrientationAttrIndex, false)
    {
      for (size_t i(0); i < 3; ++i)
	_scalarvalues[i].set_text("0.0");
      _scalarvalues[3].set_text("1.0");
    }

    virtual void bindAttribute()
    {
      Gtk::TreeModel::iterator iter = _comboBox.get_active();

      if (singleValueMode())
	{
	  setConstantAttribute(_attrnum);
	  return;
	}
      
      std::tr1::shared_ptr<Attribute> ptr = (*iter)[_modelColumns.m_ptr];	  
      if (ptr->components() == 4)
	{
	  ptr->bindAttribute(_attrnum, false);
	  return;
	}
      
      if (ptr->components() != 3)
	M_throw() << "Cannot create orientation from anything other than a 3 component Attribute";


      if ((_lastAttribute != ptr.get())
	  || (_lastAttributeDataCount != ptr->getUpdateCount())
	  || _filteredData.empty())
	{
	  _lastAttribute = ptr.get();
	  _lastAttributeDataCount = ptr->getUpdateCount();
      
	  const size_t elements = ptr->num_elements();
	  _filteredData.init(4 * elements);
	  const std::vector<GLfloat>& attrdata = *ptr;
	  GLfloat* glptr = _filteredData.map();
	  
	  for (size_t i(0); i < elements; ++i)
	    {
	      //First we use the vector and axis to calculate a
	      //rotation twice as big as is needed. 
	      Vector vec(attrdata[3 * i + 0], attrdata[3 * i + 1], attrdata[3 * i + 2]);	  
	      Vector axis(0,0,1);
	      
	      double vecnrm = vec.nrm();
	      double cosangle = (vec | axis) / vecnrm;
	      //Special case of no rotation or zero length vector
	      if ((vecnrm == 0) || (cosangle == 1))
		{
		  glptr[4 * i + 0] = glptr[4 * i + 1] = glptr[4 * i + 2] = 0;
		  glptr[4 * i + 3] = 1;
		  continue;
		}
	      //Special case where vec and axis are opposites
	      if (cosangle == -1)
		{
		  //Just rotate around the x axis by 180 degrees
		  glptr[4 * i + 0] = 1;
		  glptr[4 * i + 1] = glptr[4 * i + 2] = glptr[4 * i + 3] = 0;
		  continue;
		}
	      
	      //Calculate the rotation axis
	      Vector rot_axis = (vec ^ axis) / vecnrm;
	      for (size_t j(0); j < 3; ++j)
		glptr[4 * i + j] = rot_axis[j];
	      
	      glptr[4 * i + 3] = cosangle;
	      
	      double sum = 0;
	      for (size_t j(0); j < 4; ++j)
		sum += glptr[4 * i + j] * glptr[4 * i + j];
	      sum = std::sqrt(sum);
	      for (size_t j(0); j < 4; ++j)
		glptr[4 * i + j] /= sum;
	      
	      //As the rotation is twice as big as needed, we must do
	      //a half angle conversion.
	      glptr[4 * i + 3] += 1;
	      sum = 0;
	      for (size_t j(0); j < 4; ++j)
		sum += glptr[4 * i + j] * glptr[4 * i + j];
	      sum = std::sqrt(sum);
	      for (size_t j(0); j < 4; ++j)
		glptr[4 * i + j] /= sum;
	    }
	  
	  _filteredData.unmap();
	}
      _filteredData.attachToAttribute(_attrnum, 4, 1);
    }
  };
}