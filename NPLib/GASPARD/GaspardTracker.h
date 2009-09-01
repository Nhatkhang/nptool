#ifndef GaspardTracker_H

// NPL
#include "../include/VDetector.h"
#include "TGaspardTrackerData.h"
#include "TGaspardTrackerPhysics.h"

// Root
#include "TVector3.h"

class GaspardTracker : public NPA::VDetector
	{
			public:	//	Default Constructor
				GaspardTracker()	;
				~GaspardTracker()	;
	
			public:		//	Innherited from VDetector Class
			
				//	Read stream at ConfigFile to pick-up parameters of detector (Position,...) using Token
				void ReadConfiguration(string) 				;
				

				//	Read stream at CalibFile and pick-up calibration parameter using Token
				//	If argument is "Simulation" no change calibration is loaded
				void ReadCalibrationFile(string)	 		;		
					
				
				//	Activated associated Branches and link it to the private member DetectorData address
				//	In this method mother Branches (Detector) AND daughter leaf (fDetector_parameter) have to be activated
				void InitializeRootInput() 					;


				//	Create associated branches and associated private member DetectorPhysics address
				void InitializeRootOutput() 		 		;
				
				
				//	This method is called at each event read from the Input Tree. Aime is to build treat Raw dat in order to extract physical parameter. 
				void BuildPhysicalEvent()					;
				
				
				//	Same as above, but only the simplest event and/or simple method are used (low multiplicity, faster algorythm but less efficient ...).
				//	This method aimed to be used for analysis performed during experiment, when speed is requiered.
				//	NB: This method can eventually be the same as BuildPhysicalEvent.
				void BuildSimplePhysicalEvent()				;

				//	Those two method all to clear the Event Physics or Data
				void ClearEventPhysics()		{EventPhysics->Clear();}		
				void ClearEventData()			{EventData->Clear();}		


			public:		//	Specific to GaspardTracker
				//	Add a Telescope using Corner Coordinate information
				void AddTelescope(	TVector3 C_X1_Y1 		,
			 						TVector3 C_X128_Y1 		, 
			 						TVector3 C_X1_Y128 		, 
			 						TVector3 C_X128_Y128	);
				
				//	Add a Telescope using R Theta Phi of Si center information
				void AddTelescope(	double theta 	, 
									double phi 		, 
									double distance , 
									double beta_u 	, 
									double beta_v 	, 
									double beta_w	);
									
				double GetStripPositionX( int N , int X , int Y )	{ return StripPositionX[N-1][X-1][Y-1] ; };
				double GetStripPositionY( int N , int X , int Y )	{ return StripPositionY[N-1][X-1][Y-1] ; };
				double GetStripPositionZ( int N , int X , int Y )	{ return StripPositionZ[N-1][X-1][Y-1] ; };

				double GetNumberOfTelescope() 	{ return NumberOfTelescope ; }			;

				// To be called after a build Physical Event 
				
				double GetEnergyDeposit() ;
				
				TVector3 GetPositionOfInteraction();
				
				void Print()	;

			private:	//	Root Input and Output tree classes
				
				TGaspardTrackerData* 	EventData		;
				TGaspardTrackerPhysics* EventPhysics	;


			private:	//	Spatial Position of Strip Calculated on bases of detector position
			
				int NumberOfTelescope	;
			
				vector< vector < vector < double > > >	StripPositionX			;
				vector< vector < vector < double > > >	StripPositionY			;
				vector< vector < vector < double > > >	StripPositionZ			;

	};

#endif