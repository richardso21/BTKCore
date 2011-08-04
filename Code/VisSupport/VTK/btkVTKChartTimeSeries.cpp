/* 
 * The Biomechanical ToolKit
 * Copyright (c) 2009-2011, Arnaud Barré
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *     * Redistributions of source code must retain the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name(s) of the copyright holders nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "btkVTKChartTimeSeries.h"
#include "btkMacro.h"

#include <vtkObjectFactory.h>
#include <vtkChartLegend.h>
#include <vtkAxis.h>
#include <vtkPlot.h>
#include <vtkPlotGrid.h>
#include <vtkColorSeries.h>
#include <vtkContextScene.h>
#include <vtkPoints2D.h>
#include <vtkContext2D.h>
#include <vtkColor.h>
#include <vtkPlotLine.h>
#include <vtkPen.h>
#include <vtkTransform2D.h>
#include <vtkContextDevice2D.h>

#include <vtkstd/list>

namespace btk
{
  /**
   * @class VTKPlots btkVTKChartTimeSeries.h
   * @brief List of pointer to vtkPlot objects.
   */
  class VTKPlots: public vtkstd::list<vtkPlot*>
  {};
  
  /**
   * @class VTKChartTimeSeries btkVTKChartTimeSeries.h
   * @brief Chart for the time series data with only a bottom and left axes.
   *
   * This chart has several interaction to constraint pan and zoom
   *  - Select only a horizontal or vertical zoom
   *  - Pan the chart only into boundaries
   *
   * You are able also to enable/disable interaction.
   * 
   * @ingroup BTKVTK
   */
   
  /**
   * @fn static VTKChartTimeSeries* VTKChartTimeSeries::New()
   * Constructs a VTKChartTimeSeries object and return it as a pointer.
   */
  vtkStandardNewMacro(VTKChartTimeSeries);
  vtkCxxRevisionMacro(VTKChartTimeSeries, "$Revision: 0.1 $");
  
  /**
   * @fn bool VTKChartTimeSeries::GetInteractionEnabled() const
   * Get the status of the user interactions (move & zoom).
   */
   
  /**
   * @fn void VTKChartTimeSeries::SetInteractionEnabled(bool enabled)
   * Enable / disable the user interactions (move & zoom).
   */
  
  /**
   * @fn int VTKChartTimeSeries::GetZoomMode() const
   * Returns the zoom mode (Both directions (BOTH): 0, only horizontal (HORIZONTAL): 1, only vertical (VERTICAL): 2)
   */
   
  /**
   * @fn void VTKChartTimeSeries::SetZoomMode(int mode)
   * Sets the zoom mode (Both directions (BOTH): 0, only horizontal (HORIZONTAL): 1, only vertical (VERTICAL): 2)
   */
  
  /**
   * @fn bool VTKChartTimeSeries::GetBoundsEnabled()
   * Get the status for the use of the bounds on the chart's axes.
   */

  /**
   * @fn void VTKChartTimeSeries::SetBoundsEnabled(bool enabled)
   * Enable / disable the use of the bounds on the chart's axes.
   */
  
  /**
   * @fn double* VTKChartTimeSeries::GetBounds()
   * Returns the boundaries set for the charts as xMin, xMax, yMin, yMax.
   *
   * If this is enabled (see the method SetBoundsEnabled()), then the zoom interaction will be limited by these bounds. 
   */
  
  /**
   * @fn void VTKChartTimeSeries::SetBounds(double xMin, double xMax, double yMin, double yMax)
   * Convenient method to set the chart's bounds.
   */
  
  /**
   * Sets the boundaries for the charts.
   * The input format is xMin, xMax, yMin, yMax.
   * Need to be enabled (see the method SetBoundsEnabled()) to constraint the interactions on the chart.
   */
  void VTKChartTimeSeries::SetBounds(double bounds[4])
  {
    if ((bounds[0] == this->mp_Bounds[0]) && (bounds[1] == this->mp_Bounds[1]) && (bounds[2] == this->mp_Bounds[2]) && (bounds[3] == this->mp_Bounds[3]))
      return;
    
    this->mp_Bounds[0] = bounds[0];
    this->mp_Bounds[1] = bounds[1];
    this->mp_Bounds[2] = bounds[2];
    this->mp_Bounds[3] = bounds[3];
    
    this->mp_AxisX->SetRange(bounds[0], bounds[1]);
    this->mp_AxisY->SetRange(bounds[2], bounds[3]);
    
    this->m_ChartBoundsValid = true;
    this->m_PlotsTransformValid = false;
    this->Scene->SetDirty(true);
  };
  
  /**
   * Recalculates the bounds of the chart, and then of its axes.
   */
  void VTKChartTimeSeries::RecalculateBounds()
  {
    this->Update();
    
    double x[2] = {std::numeric_limits<double>::max(), std::numeric_limits<double>::min()};
    double y[2] = {x[0], x[1]};
    double bounds[4] = { 0.0, 0.0, 0.0, 0.0 };
    bool validBounds = false;
    for (vtkstd::list<vtkPlot*>::iterator it = this->mp_Plots->begin() ; it != this->mp_Plots->end() ; ++it)
    {
      if (!(*it)->GetVisible())
        continue;
      (*it)->GetBounds(bounds);
      x[0] = std::min(x[0], bounds[0]);
      x[1] = std::max(x[1], bounds[1]);
      y[0] = std::min(y[0], bounds[2]);
      y[1] = std::max(y[1], bounds[3]);
      validBounds = true;
    }
    if (!validBounds) // No (visible) plot
    {
      x[0] = 0.0; x[1] = 0.0;
      y[0] = 0.0; y[1] = 0.0;
    }
    
    // The X axis is set to fit exactly the time range
    // The Y axis is larger to see correctly the minimum and maximum values.
    // +/- 5% of the range is added.
    double r = (y[1] - y[0]) / 20.0;
    y[0] -= r; y[1] += r;
    
    this->SetBounds(x[0], x[1], y[0], y[1]);
  };
  
  /**
   * Sets the legend
   */
  void VTKChartTimeSeries::SetLegend(vtkChartLegend* legend)
  {
    if (this->mp_Legend == legend)
      return;
    else if (this->mp_Legend != 0)
      this->mp_Legend->Delete();
    this->mp_Legend = legend;
    this->mp_Legend->SetChart(this);
    this->mp_Legend->Register(this);
  };
  
  /**
   * @fn vtkChartLegend* VTKChartTimeSeries::GetLegend()
   * Returns the legend (NULL by default).
   */
  
  /**
   *  Sets the generator of colors.
   */
  void VTKChartTimeSeries::SetColorSeries(vtkColorSeries* colors)
  {
    if (this->mp_Colors == colors)
      return;
    else if (this->mp_Colors != 0)
      this->mp_Colors->Delete();
    this->mp_Colors = colors;
    this->mp_Colors->Register(this);
  };
  
  /**
   * @fn vtkColorSeries* VTKChartTimeSeries::GetColorSeries()
   * Returns the generator of colors (NULL by default).
   */
  
   /**
    * Modify the size of the borders and request to update them in the next painting operation.
    */
  void VTKChartTimeSeries::SetBorders(int left, int bottom, int right, int top)
  {
    if ((this->mp_Borders[0] == left) && (this->mp_Borders[0] == left) && (this->mp_Borders[0] == left) && (this->mp_Borders[0] == left))
      return;
    this->vtkChart::SetBorders(left, bottom, right, top);
    // Set the borders value after as the method SetBorders(), check the given value and modify them if necessary
    this->mp_Borders[0] = this->Point1[0]; // Left
    this->mp_Borders[1] = this->Point1[1]; // Bottom
    this->mp_Borders[2] = this->Geometry[0] - this->Point2[0]; // Right
    this->mp_Borders[3] = this->Geometry[1] - this->Point2[1]; // Top
    this->m_BordersChanged = true;
    this->Scene->SetDirty(true);
  };
   
  /**
   * Update the content of the chart which is not graphical. This function is called by the method Paint().
   */
  void VTKChartTimeSeries::Update()
  {
    for (vtkstd::list<vtkPlot*>::iterator it = this->mp_Plots->begin() ; it != this->mp_Plots->end() ; ++it)
      (*it)->Update();
    if (this->mp_Legend && this->ShowLegend)
      this->mp_Legend->Update();
  };
  
  /**
   * Paint the contents of the chart in the scene.
   */
  bool VTKChartTimeSeries::Paint(vtkContext2D *painter)
  {
    int geometry[] = {this->GetScene()->GetSceneWidth(), this->GetScene()->GetSceneHeight()};
    // Do we have a scene with a valid geometry?
    if ((geometry[0] == 0) || (geometry[1] == 0))
      return false;
    
    // Update the content of the plots, legend, etc. 
    this->Update();
    
    // Update the chart's geometry if necessary
    if ((geometry[0] != this->Geometry[0]) || (geometry[1] != this->Geometry[1]) || this->m_BordersChanged)
    {
      this->SetGeometry(geometry);
      this->vtkChart::SetBorders(this->mp_Borders[0], this->mp_Borders[1], this->mp_Borders[2], this->mp_Borders[3]);
      // But also the axis length
      this->mp_AxisX->SetPoint1(this->Point1[0], this->Point1[1]);
      this->mp_AxisX->SetPoint2(this->Point2[0], this->Point1[1]);
      this->mp_AxisY->SetPoint1(this->Point1[0], this->Point1[1]);
      this->mp_AxisY->SetPoint2(this->Point1[0], this->Point2[1]);
      // And the legend
      if (this->mp_Legend != 0)
        this->mp_Legend->SetPoint(this->Point2[0], this->Point2[1]);
      this->m_BordersChanged = false;
      this->m_PlotsTransformValid = false;
    }
    
    // Update the axes
    if (!this->m_ChartBoundsValid)
      this->RecalculateBounds();
    if (!this->m_PlotsTransformValid)
      this->RecalculatePlotsTransform();
    this->mp_AxisX->Update();
    this->mp_AxisY->Update();
    
    // Draw the items used in the chart. The order is important.
    // I. The grid is in the back
    this->mp_Grid->Paint(painter);
    // Then the plots
    // II.1. Clip the plot between the axes
    float clipF[4] = {this->Point1[0], this->Point1[1], this->Point2[0]-this->Point1[0], this->Point2[1]-this->Point1[1] };
    // II.1.1 Check if the scene has a transform and use it
    if (this->Scene->HasTransform())
      this->Scene->GetTransform()->InverseTransformPoints(clipF, clipF, 2);
    int clip[4] = {(int)clipF[0], (int)clipF[1], (int)clipF[2], (int)clipF[3]};
    painter->GetDevice()->SetClipping(clip);
    // II.2. Plot rendering
    painter->PushMatrix();
    painter->AppendTransform(this->mp_PlotsTransform);
    for (vtkstd::list<vtkPlot*>::iterator it = this->mp_Plots->begin() ; it != this->mp_Plots->end() ; ++it)
      (*it)->Paint(painter);
    painter->PopMatrix();
    // II.3. Disable cliping
    painter->GetDevice()->DisableClipping();
    // III. The axes
    painter->GetPen()->SetColorF(0.0, 0.0, 0.0, 1.0);
    painter->GetPen()->SetWidth(1.0);
    this->mp_AxisX->Paint(painter);
    this->mp_AxisY->Paint(painter);
    // IV. The legend
    if (this->mp_Legend && this->ShowLegend)
      this->mp_Legend->Paint(painter);
    // V. The title
    if (this->Title)
    {
      vtkPoints2D* rect = vtkPoints2D::New();
      rect->InsertNextPoint(this->Point1[0], this->Point2[1]);
      rect->InsertNextPoint(this->Point2[0]-this->Point1[0], 10);
      painter->ApplyTextProp(this->TitleProperties);
      painter->DrawStringRect(rect, this->Title);
      rect->Delete();
    }
    
    return true;
  };
  
  /**
   * Add a new plot to the chart. Only the type LINE is supported.
   */
  vtkPlot* VTKChartTimeSeries::AddPlot(int type)
  {
    vtkPlot* plot = NULL;
    vtkColor3ub color(0,0,0); // Black by default
    if (this->mp_Colors != 0)
      color = this->mp_Colors->GetColorRepeating(static_cast<int>(this->mp_Plots->size()));
    switch (type)
    {
    case LINE:
      {
      vtkPlotLine *line = vtkPlotLine::New();
      line->GetPen()->SetColor(color.GetData());
      plot = line;
      break;
      }
    case POINTS:
    case BAR:
    default:
      btkErrorMacro("Only the plot type LINE is supported by this chart");
    }
    if (plot)
    {
      plot->SetXAxis(this->mp_AxisX);
      plot->SetYAxis(this->mp_AxisY);
      this->mp_Plots->push_back(plot);
      // Ensure that the bounds of the chart are updated to contain the new plot
      this->m_ChartBoundsValid = false;
      // Mark the scene as dirty to update it.
      this->Scene->SetDirty(true);
    }
    return plot;
  };
  
  /**
   * Removes a plot with the index @a index and request to update the bounds.
   */
  bool VTKChartTimeSeries::RemovePlot(vtkIdType index)
  {
    if (static_cast<vtkIdType>(this->mp_Plots->size()) > index)
    {
      vtkstd::list<vtkPlot*>::iterator it = this->mp_Plots->begin();
      vtkstd::advance(it, index);
      (*it)->Delete();
      this->mp_Plots->erase(it);
      // Ensure that the bounds of the chart are updated to fit well the plots
      this->m_ChartBoundsValid = false;
      // Mark the scene as dirty
      this->Scene->SetDirty(true);
      return true;
    }
    return false;
  };
  
  /**
   * Removes all the plots and reset all the bounds to 0
   */
  void VTKChartTimeSeries::ClearPlots()
  {
    for (vtkstd::list<vtkPlot*>::iterator it = this->mp_Plots->begin() ; it != this->mp_Plots->end() ; ++it)
      (*it)->Delete();
    this->mp_Plots->clear();
    this->m_ChartBoundsValid = false;
    this->Scene->SetDirty(true);
  };
  
  /**
   * Returns the plot at the index @a index.
   */
  vtkPlot* VTKChartTimeSeries::GetPlot(vtkIdType index)
  {
    vtkstd::list<vtkPlot*>::iterator it = this->mp_Plots->begin();
    vtkstd::advance(it, index);
    if (it == this->mp_Plots->end())
    {
      btkErrorMacro("The given index exceed the number of plots");
      return NULL;
    }
    return *it;
  };
  
  /**
   * Returns the number of plots in the chart.
   */
  vtkIdType VTKChartTimeSeries::GetNumberOfPlots()
  {
    return this->mp_Plots->size();
  };
  
  /**
   * Returns the axis baed on the index @a axisIndex.
   * Only the axes BOTTOM and LEFT are supported.
   */
  vtkAxis* VTKChartTimeSeries::GetAxis(int axisIndex)
  {
    vtkAxis* axis = 0;
    switch (axisIndex)
    {
    case 0: // Left
      axis = this->mp_AxisY;
      break;
    case 1: // Bottom
      axis = this->mp_AxisX;
      break;
    default:
      btkErrorMacro("Only two axes are available with this chart: LEFT and BOTTOM.");
    }
    return axis;
  };
  
  /**
   * Returns the number of axes in the chart.
   */
  vtkIdType VTKChartTimeSeries::GetNumberOfAxes()
  {
    return 2;
  };
  
  /**
   * Return true if the supplied x, y coordinate is inside the item.
   * Required for the MouseWheelEvent() method.
   */
  bool VTKChartTimeSeries::Hit(const vtkContextMouseEvent &mouse)
  {
    if ((mouse.ScreenPos[0] > this->Point1[0]) && (mouse.ScreenPos[0] < this->Point2[0])
     && (mouse.ScreenPos[1] > this->Point1[1]) && (mouse.ScreenPos[1] < this->Point2[1]))
      return true;
    else
      return false;
  };
  
  /**
   * Overloaded method to move the chart only if the user interaction are enabled.
   */
  bool VTKChartTimeSeries::MouseMoveEvent(const vtkContextMouseEvent& mouse)
  {
    if (this->m_InteractionEnabled)
    {
      for (int i = 0 ; i < 2 ; ++i)
      {
        int mai = 1-i; // Map between the axis and the corresponding index in the array
        vtkAxis* axis = this->GetAxis(mai);
        float pt1[2]; axis->GetPoint1(pt1);
        float pt2[2]; axis->GetPoint2(pt2);
        double min = axis->GetMinimum(); double max = axis->GetMaximum();
        double scale = (max - min) / (double)(pt2[i] - pt1[i]);
        double delta = (mouse.LastScreenPos[i] - mouse.ScreenPos[i]) * scale;
        
        min = axis->GetMinimum() + delta;
        max = axis->GetMaximum() + delta;
        if (this->m_BoundsEnabled && (min < this->mp_Bounds[i*2]))
        {
          min = this->mp_Bounds[i*2];
          max = axis->GetMaximum();
        }
        else if (this->m_BoundsEnabled && (max > this->mp_Bounds[i*2+1]))
        {
          min = axis->GetMinimum();
          max = this->mp_Bounds[i*2+1];
        }
        axis->SetRange(min, max);
        // Because we forces the opposite axes to have the same range it is not necessary to recompute the delta.
        // this->GetAxis(mai+2)->SetRange(min, max);
      }
      
      // this->RecalculatePlotsTransform();
      this->m_PlotsTransformValid = false;
      this->Scene->SetDirty(true);
    }
    return true;
  };
  
  /**
   * Overloaded method to zoom in/out on the axis(es) specified by the zoom mode.
   */
  bool VTKChartTimeSeries::MouseWheelEvent(const vtkContextMouseEvent& mouse, int delta)
  {
    btkNotUsed(mouse);
    
    if (this->m_InteractionEnabled)
    {
      for (int i = 0 ; i < 2 ; ++i)
      {
        int mai = 1-i; // Map between the axis and the corresponding index in the array
        if ((this->m_ZoomMode == i) || (this->m_ZoomMode == BOTH)) // i=0: Horizontal, i=1: Vertical.
        {
          vtkAxis* axis = this->GetAxis(mai);
          double min = axis->GetMinimum();
          double max = axis->GetMaximum();
          double frac = (max - min) * 0.05;
          min += delta*frac;
          max -= delta*frac;
          if (this->m_BoundsEnabled)
          {
            if (min < this->mp_Bounds[i*2])
              min = this->mp_Bounds[i*2];
            if (max > this->mp_Bounds[i*2+1])
              max = this->mp_Bounds[i*2+1];
          }
          axis->SetRange(min,max);
          // vtkAxis* axis2 = this->GetAxis(mai+2);
          // axis2->SetRange(min,max);
          axis->RecalculateTickSpacing();
          // axis2->RecalculateTickSpacing();
        }
      }
      // this->RecalculatePlotsTransform();
      this->m_PlotsTransformValid = false;
      this->Scene->SetDirty(true);
    }
    return true;
  };
  
  VTKChartTimeSeries::VTKChartTimeSeries()
  : vtkChart()
  {
    // No legend by defaut.
    this->mp_Legend = 0;
    
    // No generator of color by default
    this->mp_Colors = 0;
    
    // Only two axes (bottom: X axis, left: Y axis)
    this->mp_AxisX = vtkAxis::New();
    this->mp_AxisX->SetPosition(vtkAxis::BOTTOM);
    this->mp_AxisX->SetTitle("X Axis");
    this->mp_AxisX->SetVisible(true);
    this->mp_AxisY = vtkAxis::New();
    this->mp_AxisY->SetPosition(vtkAxis::LEFT);
    this->mp_AxisY->SetTitle("Y Axis");
    this->mp_AxisY->SetVisible(true);
    // By default, they will be displayed with a null range and their behavior is fixed
    this->mp_AxisX->SetBehavior(1); // Fixed
    this->mp_AxisX->SetRange(0.0, 0.0);
    this->mp_AxisY->SetBehavior(1); // Fixed
    this->mp_AxisY->SetRange(0.0, 0.0);
    
    // Grid using the defined axes
    this->mp_Grid = vtkPlotGrid::New();
    this->mp_Grid->SetXAxis(this->mp_AxisX);
    this->mp_Grid->SetYAxis(this->mp_AxisY);
    
    // Interaction and bounds
    this->m_InteractionEnabled = true;
    this->m_ZoomMode = 0;
    this->m_BoundsEnabled = false;
    this->mp_Bounds[0] = 0.0;
    this->mp_Bounds[1] = 0.0;
    this->mp_Bounds[2] = 0.0;
    this->mp_Bounds[3] = 0.0;
    this->m_ChartBoundsValid = true;
    
    // Borders
    this->mp_Borders[0] = 0;
    this->mp_Borders[1] = 0;
    this->mp_Borders[2] = 0;
    this->mp_Borders[3] = 0;
    
    // Plots
    this->mp_Plots = new VTKPlots();
    
    // Linear transformation to scale and translate the plots
    this->mp_PlotsTransform = vtkTransform2D::New();
    this->m_PlotsTransformValid = true;
    
    // Defaut borders for the chart
    this->m_BordersChanged = false;
    this->SetBorders(60, 50, 20, 20); 
  };
  
  VTKChartTimeSeries::~VTKChartTimeSeries()
  {
    for (vtkstd::list<vtkPlot*>::iterator it = this->mp_Plots->begin() ; it != this->mp_Plots->end() ; ++it)
      (*it)->Delete();
    delete this->mp_Plots;
    if (this->mp_Legend)
      this->mp_Legend->Delete();
    if (this->mp_Legend)
      this->mp_Legend->Delete();
    this->mp_AxisX->Delete();
    this->mp_AxisY->Delete();
    this->mp_Grid->Delete();
    this->mp_PlotsTransform->Delete();
  };
  
  /**
   * Update the size of the plot into the dimensions of the scene.
   */
  void VTKChartTimeSeries::RecalculatePlotsTransform()
  {
    // Compute the scales for the plot area to fit the plot inside
    float min[2], max[2];
    // Axis X
    if (this->mp_AxisX->GetMaximum() == this->mp_AxisX->GetMinimum())
      return;
    this->mp_AxisX->GetPoint1(min);
    this->mp_AxisX->GetPoint2(max);
    double scaleX = static_cast<double>(max[0] - min[0]) / (this->mp_AxisX->GetMaximum() - this->mp_AxisX->GetMinimum());
    // Axis Y
    if (this->mp_AxisY->GetMaximum() == this->mp_AxisY->GetMinimum())
      return;
    this->mp_AxisY->GetPoint1(min);
    this->mp_AxisY->GetPoint2(max);
    double scaleY = static_cast<double>(max[1] - min[1]) / (this->mp_AxisY->GetMaximum() - this->mp_AxisY->GetMinimum());

    this->mp_PlotsTransform->Identity();
    this->mp_PlotsTransform->Translate(this->Point1[0], this->Point1[1]);
    this->mp_PlotsTransform->Scale(scaleX, scaleY);
    this->mp_PlotsTransform->Translate(-this->mp_AxisX->GetMinimum(), -this->mp_AxisY->GetMinimum());
    
    this->m_PlotsTransformValid = true;
  };
};