/* ============================================================================
 * Copyright (c) 2014 DREAM3D Consortium
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the names of any of the DREAM3D Consortium contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  This code was partially written under United States Air Force Contract number
 *                              FA8650-10-D-5210
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include "RegionGrowing.h"

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/Geometry/ImageGeom.h"

#include "ItkBridge.h"

//thresholding filter
#include "itkBinaryThresholdImageFilter.h"
#include "itkConfidenceConnectedImageFilter.h"


// Include the MOC generated file for this class
#include "moc_RegionGrowing.cpp"



// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
RegionGrowing::RegionGrowing() :
  AbstractFilter(),
  m_SelectedCellArrayPath("", "", ""),
  m_NewCellArrayName(""),
  m_SaveAsNewArray(true),
  m_ManualParameter(128),
  m_SelectedCellArrayArrayName(""),
  m_SelectedCellArray(NULL),
  m_NewCellArray(NULL)
{
  setupFilterParameters();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
RegionGrowing::~RegionGrowing()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegionGrowing::setupFilterParameters()
{
  FilterParameterVector parameters;
  parameters.push_back(DataArraySelectionFilterParameter::New("Array to Process", "SelectedCellArrayPath", getSelectedCellArrayPath(), FilterParameter::Uncategorized));
  QStringList linkedProps;
  linkedProps << "NewCellArrayName";
  parameters.push_back(LinkedBooleanFilterParameter::New("Save As New Array", "SaveAsNewArray", getSaveAsNewArray(), linkedProps, FilterParameter::Uncategorized, SIMPL_BIND_SETTER(ItkRegionGrowing, this, SaveAsNewArray), SIMPL_BIND_GETTER(ItkRegionGrowing, this, SaveAsNewArray)));
  parameters.push_back(StringFilterParameter::New("Created Array Name", "NewCellArrayName", getNewCellArrayName(), FilterParameter::Uncategorized, SIMPL_BIND_SETTER(ItkRegionGrowing, this, NewCellArrayName), SIMPL_BIND_GETTER(ItkRegionGrowing, this, NewCellArrayName)));
  parameters.push_back(IntFilterParameter::New("Threshold Value", "ManualParameter", getManualParameter(), FilterParameter::Uncategorized, SIMPL_BIND_SETTER(ItkRegionGrowing, this, ManualParameter), SIMPL_BIND_GETTER(ItkRegionGrowing, this, ManualParameter)));
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegionGrowing::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setSelectedCellArrayPath( reader->readDataArrayPath( "SelectedCellArrayPath", getSelectedCellArrayPath() ) );
  setNewCellArrayName( reader->readString( "NewCellArrayName", getNewCellArrayName() ) );
  setSaveAsNewArray( reader->readValue( "SaveAsNewArray", getSaveAsNewArray() ) );
  setManualParameter( reader->readValue( "ManualParameter", getManualParameter() ) );
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int RegionGrowing::writeFilterParameters(AbstractFilterParametersWriter* writer, int index)
{
  writer->openFilterGroup(this, index);
  SIMPL_FILTER_WRITE_PARAMETER(SelectedCellArrayPath)
  SIMPL_FILTER_WRITE_PARAMETER(NewCellArrayName)
  SIMPL_FILTER_WRITE_PARAMETER(SaveAsNewArray)
  SIMPL_FILTER_WRITE_PARAMETER(ManualParameter)
  writer->closeFilterGroup();
  return ++index; // we want to return the next index that was just written to
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegionGrowing::initialize()
{

}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegionGrowing::dataCheck()
{
  setErrorCondition(0);
  DataArrayPath tempPath;

  QVector<size_t> dims(1, 1);
  m_SelectedCellArrayPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<ImageProcessingConstants::DefaultPixelType>, AbstractFilter>(this, getSelectedCellArrayPath(), dims); /* Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
  if( NULL != m_SelectedCellArrayPtr.lock().get() ) /* Validate the Weak Pointer wraps a non-NULL pointer to a DataArray<T> object */
  { m_SelectedCellArray = m_SelectedCellArrayPtr.lock()->getPointer(0); } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCondition() < 0) { return; }

  ImageGeom::Pointer image = getDataContainerArray()->getDataContainer(getSelectedCellArrayPath().getDataContainerName())->getPrereqGeometry<ImageGeom, AbstractFilter>(this);
  if(getErrorCondition() < 0 || NULL == image.get()) { return; }

  if(m_SaveAsNewArray == false) { m_NewCellArrayName = "thisIsATempName"; }
  tempPath.update(getSelectedCellArrayPath().getDataContainerName(), getSelectedCellArrayPath().getAttributeMatrixName(), getNewCellArrayName() );
  m_NewCellArrayPtr = getDataContainerArray()->createNonPrereqArrayFromPath<DataArray<ImageProcessingConstants::DefaultPixelType>, AbstractFilter, ImageProcessingConstants::DefaultPixelType>(this, tempPath, 0, dims); /* Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
  if( NULL != m_NewCellArrayPtr.lock().get() ) /* Validate the Weak Pointer wraps a non-NULL pointer to a DataArray<T> object */
  { m_NewCellArray = m_NewCellArrayPtr.lock()->getPointer(0); } /* Now assign the raw pointer to data from the DataArray<T> object */
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegionGrowing::preflight()
{
  // These are the REQUIRED lines of CODE to make sure the filter behaves correctly
  setInPreflight(true); // Set the fact that we are preflighting.
  emit preflightAboutToExecute(); // Emit this signal so that other widgets can do one file update
  emit updateFilterParameters(this); // Emit this signal to have the widgets push their values down to the filter
  dataCheck(); // Run our DataCheck to make sure everthing is setup correctly
  emit preflightExecuted(); // We are done preflighting this filter
  setInPreflight(false); // Inform the system this filter is NOT in preflight mode anymore.
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegionGrowing::execute()
{
  //int err = 0;
  dataCheck();
  if(getErrorCondition() < 0) { return; }

  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getSelectedCellArrayPath().getDataContainerName());
  QString attrMatName = getSelectedCellArrayPath().getAttributeMatrixName();

  //get dims
  size_t udims[3] = {0, 0, 0};
  m->getGeometryAs<ImageGeom>()->getDimensions(udims);


  //wrap input as itk image
  ImageProcessingConstants::DefaultImageType::Pointer inputImage = ITKUtilitiesType::CreateItkWrapperForDataPointer(m, attrMatName, m_SelectedCellArray);


  typedef itk::ConfidenceConnectedImageFilter<ImageProcessingConstants::DefaultImageType, ImageProcessingConstants::DefaultImageType> ConfidenceConnectedFilterType;
  ConfidenceConnectedFilterType::Pointer confidenceConnectedFilter = ConfidenceConnectedFilterType::New();
  confidenceConnectedFilter->SetInitialNeighborhoodRadius(3);
  confidenceConnectedFilter->SetMultiplier(3);
  confidenceConnectedFilter->SetNumberOfIterations(25);
  confidenceConnectedFilter->SetReplaceValue(255);

  // Set seed
  ImageType::IndexType seed;
  seed[0] = 500;
  seed[1] = 500;
  confidenceConnectedFilter->AddSeed(seed);
  confidenceConnectedFilter->SetInput(inputImage);

  try
  {
    confidenceConnectedFilter->Update();
  }
  catch( itk::ExceptionObject& err )
  {
    setErrorCondition(-5);
    QString ss = QObject::tr("Failed to execute itk::RegionGrowing filter. Error Message returned from ITK:\n   %1").arg(err.GetDescription());
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
  }

  //array name changing/cleanup
  if(m_SaveAsNewArray == false)
  {
    AttributeMatrix::Pointer attrMat = m->getAttributeMatrix(m_SelectedCellArrayPath.getAttributeMatrixName());
    attrMat->removeAttributeArray(m_SelectedCellArrayPath.getDataArrayName());
    attrMat->renameAttributeArray(m_NewCellArrayName, m_SelectedCellArrayPath.getDataArrayName());
  }

  /* Let the GUI know we are done with this filter */
  notifyStatusMessage(getHumanLabel(), "Complete");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer RegionGrowing::newFilterInstance(bool copyFilterParameters)
{
  RegionGrowing::Pointer filter = RegionGrowing::New();
  if(true == copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString RegionGrowing::getCompiledLibraryName()
{return ImageProcessingConstants::ImageProcessingBaseName;}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString RegionGrowing::getGroupName()
{return SIMPL::FilterGroups::Unsupported;}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString RegionGrowing::getSubGroupName()
{return "Misc";}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString RegionGrowing::getHumanLabel()
{return "ITK: Region Growing";}

