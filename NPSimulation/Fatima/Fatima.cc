/*****************************************************************************
 * Copyright (C) 2009-2014   this file is part of the NPTool Project         *
 *                                                                           *
 * For the licensing terms see $NPTOOL/Licence/NPTool_Licence                *
 * For the list of contributors see $NPTOOL/Licence/Contributors             *
 *****************************************************************************/

/*****************************************************************************
 * Original Author: M. Labiche  contact address: marc.labiche@stfc.ac.uk     *
 *                                                                           *
 * Creation Date  : December 2009                                            *
 * Last update    : December 2014                                            *
 *---------------------------------------------------------------------------*
 * Decription:                                                               *
 *  This class describe the Fatima scintillator array                         *
 *                                                                           *
 *---------------------------------------------------------------------------*
 * Comment:                                                                  *
 *                                                                           *
 *****************************************************************************/

// C++ headers
#include <sstream>
#include <cmath>
#include <limits>
using namespace std;

//Geant4
#include "G4VSolid.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4UnionSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4SDManager.hh"
#include "G4Transform3D.hh"
#include "G4PVPlacement.hh"
#include "G4Colour.hh"

// NPS
#include "Fatima.hh"
using namespace FATIMA;

#include "CalorimeterScorers.hh"
#include "MaterialManager.hh"
// NPL
#include "NPOptionManager.h"
#include "RootOutput.h"

// CLHEP header
#include "CLHEP/Random/RandGauss.h"
using namespace CLHEP;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// Fatima Specific Method
Fatima::Fatima(){
  m_Event = new TFatimaData();

  // Blue
  m_LaBr3VisAtt = new G4VisAttributes(G4Colour(0, 0.5, 1));

  // Dark Grey
  m_PMTVisAtt = new G4VisAttributes(G4Colour(0.1, 0.1, 0.1));

  // Grey wireframe
  m_DetectorCasingVisAtt = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5,0.2));

  m_LogicalDetector = 0;
  m_LaBr3Scorer = 0 ;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
Fatima::~Fatima(){
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void Fatima::AddDetector(G4ThreeVector Pos1, G4ThreeVector Pos2, G4ThreeVector Pos3, G4ThreeVector Pos4){
  G4ThreeVector Pos=(Pos1+Pos2+Pos3+Pos4)/4.;
  G4ThreeVector u = Pos1-Pos2;
  G4ThreeVector v = Pos1-Pos4;
  u = u.unit(); v = v.unit();
  G4ThreeVector w = Pos.unit();
  Pos = Pos + w*Length*0.5;

  m_Pos.push_back(Pos);
  m_Rot.push_back(new G4RotationMatrix(u,v,w));
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void Fatima::AddDetector(G4ThreeVector Pos, double beta_u, double beta_v, double beta_w){
  double Theta = Pos.theta();
  double Phi = Pos.phi();

  // vector parallel to one axis of silicon plane
  G4double ii = cos(Theta / rad) * cos(Phi / rad);
  G4double jj = cos(Theta / rad) * sin(Phi / rad);
  G4double kk = -sin(Theta / rad);
  G4ThreeVector Y = G4ThreeVector(ii, jj, kk);

  G4ThreeVector w = Pos.unit();
  G4ThreeVector u = w.cross(Y);
  G4ThreeVector v = w.cross(u);
  v = v.unit();
  u = u.unit();

  G4RotationMatrix* r = new G4RotationMatrix(u,v,w);
  r->rotate(beta_u,u);
  r->rotate(beta_v,v);
  r->rotate(beta_w,w);

  m_Pos.push_back(Pos);
  m_Rot.push_back(r);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// Virtual Method of NPS::VDetector class
// Read stream at Configfile to pick-up parameters of detector (Position,...)
// Called in DetecorConstruction::ReadDetextorConfiguration Method
void Fatima::ReadConfiguration(string Path){
  ifstream ConfigFile;
  ConfigFile.open(Path.c_str());
  string LineBuffer, DataBuffer; 

  // A,B,C,D are the four corner of the detector

  G4double Ax , Bx , Cx , Dx , Ay , By , Cy , Dy , Az , Bz , Cz , Dz          ;
  G4ThreeVector A , B , C , D                                                 ;
  G4double Theta = 0 , Phi = 0 , R = 0 , beta_u = 0 , beta_v = 0 , beta_w = 0 ;
  
  bool ReadingStatus = false;

  bool check_A = false;
  bool check_C = false;
  bool check_B = false;
  bool check_D = false;

  bool check_Theta = false;
  bool check_Phi   = false;
  bool check_R     = false;

  while (!ConfigFile.eof()) {
    getline(ConfigFile, LineBuffer);
    if (LineBuffer.compare(0, 14, "FatimaDetector") == 0) {
      G4cout << "///" << G4endl           ;
      G4cout << "Detector found: " << G4endl   ;
      ReadingStatus = true ;
    }


    while (ReadingStatus) {
      ConfigFile >> DataBuffer;
      // Comment Line 
      if (DataBuffer.compare(0, 1, "%") == 0) {/*do nothing */;}

      // Position method
      else if (DataBuffer == "A=") {
        check_A = true;
        ConfigFile >> DataBuffer ;
        Ax = atof(DataBuffer.c_str()) ;
        Ax = Ax * mm ;
        ConfigFile >> DataBuffer ;
        Ay = atof(DataBuffer.c_str()) ;
        Ay = Ay * mm ;
        ConfigFile >> DataBuffer ;
        Az = atof(DataBuffer.c_str()) ;
        Az = Az * mm ;

        A = G4ThreeVector(Ax, Ay, Az);
        G4cout << "Corner A position : " << A << G4endl;
      }
      else if (DataBuffer == "B=") {
        check_B = true;
        ConfigFile >> DataBuffer ;
        Bx = atof(DataBuffer.c_str()) ;
        Bx = Bx * mm ;
        ConfigFile >> DataBuffer ;
        By = atof(DataBuffer.c_str()) ;
        By = By * mm ;
        ConfigFile >> DataBuffer ;
        Bz = atof(DataBuffer.c_str()) ;
        Bz = Bz * mm ;

        B = G4ThreeVector(Bx, By, Bz);
        G4cout << "Corner B position : " << B << G4endl;
      }

      else if (DataBuffer == "C=") {
        check_C = true;
        ConfigFile >> DataBuffer ;
        Cx = atof(DataBuffer.c_str()) ;
        Cx = Cx * mm ;
        ConfigFile >> DataBuffer ;
        Cy = atof(DataBuffer.c_str()) ;
        Cy = Cy * mm ;
        ConfigFile >> DataBuffer ;
        Cz = atof(DataBuffer.c_str()) ;
        Cz = Cz * mm ;

        C = G4ThreeVector(Cx, Cy, Cz);
        G4cout << "Corner C position : " << C << G4endl;
      }
      else if (DataBuffer == "D=") {
        check_D = true;
        ConfigFile >> DataBuffer ;
        Dx = atof(DataBuffer.c_str()) ;
        Dx = Dx * mm ;
        ConfigFile >> DataBuffer ;
        Dy = atof(DataBuffer.c_str()) ;
        Dy = Dy * mm ;
        ConfigFile >> DataBuffer ;
        Dz = atof(DataBuffer.c_str()) ;
        Dz = Dz * mm ;

        D = G4ThreeVector(Dx, Dy, Dz);
        G4cout << "Corner D position : " << D << G4endl;
      }

      // Angle method
      else if (DataBuffer=="Theta=") {
        check_Theta = true;
        ConfigFile >> DataBuffer ;
        Theta = atof(DataBuffer.c_str()) ;
        Theta = Theta * deg;
        G4cout << "Theta:  " << Theta / deg << G4endl;
      }
      else if (DataBuffer=="Phi=") {
        check_Phi = true;
        ConfigFile >> DataBuffer ;
        Phi = atof(DataBuffer.c_str()) ;
        Phi = Phi * deg;
        G4cout << "Phi:  " << Phi / deg << G4endl;
      }
      else if (DataBuffer=="R=") {
        check_R = true;
        ConfigFile >> DataBuffer ;
        R = atof(DataBuffer.c_str()) ;
        R = R * mm;
        G4cout << "R:  " << R / mm << G4endl;
      }
      else if (DataBuffer=="Beta=") {
        ConfigFile >> DataBuffer ;
        beta_u = atof(DataBuffer.c_str()) ;
        beta_u = beta_u * deg   ;
        ConfigFile >> DataBuffer ;
        beta_v = atof(DataBuffer.c_str()) ;
        beta_v = beta_v * deg   ;
        ConfigFile >> DataBuffer ;
        beta_w = atof(DataBuffer.c_str()) ;
        beta_w = beta_w * deg   ;
        G4cout << "Beta:  " << beta_u / deg << " " << beta_v / deg << " " << beta_w / deg << G4endl  ;
      }

      else G4cout << "WARNING: Wrong Token, Fatima: Dector not added" << G4endl;

      // Add The previously define telescope
      // With position method
      if ((check_A && check_B && check_C && check_D) && 
          !(check_Theta && check_Phi && check_R)) {
        ReadingStatus = false;
        check_A = false;
        check_C = false;
        check_B = false;
        check_D = false;
        
        AddDetector(A, B, C, D);
      }

      // With angle method
      if ((check_Theta && check_Phi && check_R ) && 
          !(check_A && check_B && check_C && check_D)) {
        ReadingStatus = false;
        check_Theta = false;
        check_Phi   = false;
        check_R     = false;

        R = R +  0.5*Length;
        G4ThreeVector Pos(R*sin(Theta)*cos(Phi),R*sin(Theta)*sin(Phi),R*cos(Theta));
        AddDetector(Pos, beta_u, beta_v, beta_w);
      }
    }
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// Construct detector and inialise sensitive part.
// Called After DetecorConstruction::AddDetector Method
void Fatima::ConstructDetector(G4LogicalVolume* world){
  unsigned int mysize = m_Pos.size();
  for(unsigned int i = 0 ; i < mysize ; i++){
    new G4PVPlacement(G4Transform3D(*m_Rot[i], m_Pos[i]), ConstructDetector(),  "FatimaDetector", world, false, i+1); 
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4LogicalVolume* Fatima::ConstructDetector(){
  if(!m_LogicalDetector){
    
    G4Material* Vacuum = MaterialManager::getInstance()->GetMaterialFromLibrary("Vacuum");
    G4Material* Alu = MaterialManager::getInstance()->GetMaterialFromLibrary("Al");
    G4Material* Lead = MaterialManager::getInstance()->GetMaterialFromLibrary("Pb");
    G4Material* LaBr3 = MaterialManager::getInstance()->GetMaterialFromLibrary("LaBr3");

    // Mother Volume
    G4Tubs* solidFatimaDetector = 
      new G4Tubs("Fatima",0, 0.5*FaceFront, 0.5*Length, 0.*deg, 360.*deg);
    m_LogicalDetector = 
      new G4LogicalVolume(solidFatimaDetector, Vacuum, "Fatima", 0, 0, 0);

    m_LogicalDetector->SetVisAttributes(G4VisAttributes::Invisible);

    // Detector construction
    // LaBr3
    G4ThreeVector  positionLaBr3 = G4ThreeVector(0, 0, LaBr3_PosZ);

    G4Tubs* solidLaBr3 = new G4Tubs("solidLaBr3", 0., 0.5*LaBr3Face, 0.5*LaBr3Thickness, 0.*deg, 360.*deg);
    G4LogicalVolume* logicLaBr3 = new G4LogicalVolume(solidLaBr3, LaBr3, "logicLaBr3", 0, 0, 0);

    new G4PVPlacement(0, 
        positionLaBr3, 
        logicLaBr3, 
        "Fatima_LaBr3", 
        m_LogicalDetector, 
        false, 
        0);

    // Set LaBr3 sensible
    logicLaBr3->SetSensitiveDetector(m_LaBr3Scorer);

    // Visualisation of LaBr3 Strip
    logicLaBr3->SetVisAttributes(m_LaBr3VisAtt);

    // Aluminium can around LaBr3
    // LaBr3 Can
    G4ThreeVector  positionLaBr3Can = G4ThreeVector(0, 0, LaBr3Can_PosZ);

    G4Tubs* solidLaBr3Can = new G4Tubs("solidLaBr3Can", 0.5*CanInnerDiameter, 0.5*CanOuterDiameter, 0.5*CanLength, 0.*deg, 360.*deg);
    G4LogicalVolume* logicLaBr3Can = new G4LogicalVolume(solidLaBr3Can, Alu, "logicLaBr3Can", 0, 0, 0);

    new G4PVPlacement(0, 
        positionLaBr3Can, 
        logicLaBr3Can, 
        "Fatima_LaBr3Can", 
        m_LogicalDetector, 
        false, 
        0);

    // Visualisation of LaBr3Can
    logicLaBr3Can->SetVisAttributes(m_DetectorCasingVisAtt);

    // Aluminium window in front of LaBr3
    // LaBr3 Window
    G4ThreeVector  positionLaBr3Win = G4ThreeVector(0, 0, LaBr3Win_PosZ);

    G4Tubs* solidLaBr3Win = new G4Tubs("solidLaBr3Win", 0.5*WinInnerDiameter, 0.5*WinOuterDiameter, 0.5*WinLength, 0.*deg, 360.*deg);
    G4LogicalVolume* logicLaBr3Win = new G4LogicalVolume(solidLaBr3Win, Alu, "logicLaBr3Win", 0, 0, 0);

    new G4PVPlacement(0, 
        positionLaBr3Win, 
        logicLaBr3Win, 
        "Fatima_LaBr3Win", 
        m_LogicalDetector, 
        false, 
        0);

    // Visualisation of LaBr3Win
    logicLaBr3Win->SetVisAttributes(m_DetectorCasingVisAtt);

    // PMT
    G4ThreeVector  positionPMT = G4ThreeVector(0, 0, PMT_PosZ);

    G4Tubs* solidPMout = new G4Tubs("solidPMOut", 0.5*LaBr3Face, 0.5*PMTFace, 0.5*PMTThickness, 0.*deg, 360.*deg);
    G4Tubs* solidPMin = new G4Tubs("solidPMIn", 0.5*LaBr3Face-0.1*cm, 0.5*PMTFace-0.5*cm, 0.5*(PMTThickness-2.*cm)-0.1*cm, 0.*deg, 360.*deg);
    G4RotationMatrix* RotMat=NULL;
    const G4ThreeVector &Trans= G4ThreeVector(0.,0.,1.*cm); 
    G4SubtractionSolid*           solidPMT = new G4SubtractionSolid("solidPMT", solidPMout,solidPMin, RotMat, Trans);

    G4LogicalVolume* logicPMT = new G4LogicalVolume(solidPMT, Alu, "logicPMT", 0, 0, 0);

    new G4PVPlacement(0, 
        positionPMT, 
        logicPMT, 
        "Fatima_PMT", 
        m_LogicalDetector, 
        false, 
        0);

    // Visualisation of PMT Strip
    logicPMT->SetVisAttributes(m_PMTVisAtt);

    // Lead shielding
    // A
    G4ThreeVector  positionLeadAShield = G4ThreeVector(0, 0, LeadAShield_PosZ);
    G4Tubs* solidLeadA = new G4Tubs("solidLead", 0.5*LeadAMinR, 0.5*LeadAMaxR, 0.5*LeadALength, 0.*deg, 360.*deg);
    G4LogicalVolume* logicLeadAShield = new G4LogicalVolume(solidLeadA, Lead, "logicLeadAShield", 0, 0, 0);

    new G4PVPlacement(0, 
        positionLeadAShield, 
        logicLeadAShield, 
        "Fatima_LeadAShield", 
        m_LogicalDetector, 
        false, 
        0);
    // B
    G4ThreeVector  positionLeadBShield = G4ThreeVector(0, 0, LeadBShield_PosZ);
    G4Tubs*           solidLeadB = new G4Tubs("solidLead", 0.5*LeadBMinR, 0.5*LeadBMaxR, 0.5*LeadBLength, 0.*deg, 360.*deg);
    G4LogicalVolume* logicLeadBShield = new G4LogicalVolume(solidLeadB, Lead, "logicLeadBShield", 0, 0, 0);

    new G4PVPlacement(0, 
        positionLeadBShield, 
        logicLeadBShield, 
        "Fatima_LeadBShield", 
        m_LogicalDetector, 
        false, 
        0);

    // Visualisation of PMT Strip
    G4VisAttributes* LeadVisAtt = new G4VisAttributes(G4Colour(1., 1., 0.));
    logicLeadAShield->SetVisAttributes(LeadVisAtt);
    logicLeadBShield->SetVisAttributes(LeadVisAtt);
  }

  return m_LogicalDetector;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// Add Detector branch to the EventTree.
// Called After DetecorConstruction::AddDetector Method
void Fatima::InitializeRootOutput(){
  RootOutput *pAnalysis = RootOutput::getInstance();
  TTree *pTree = pAnalysis->GetTree();
  pTree->Branch("Fatima", "TFatimaData", &m_Event) ;
  pTree->SetBranchAddress("Fatima", &m_Event) ;
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// Read sensitive part and fill the Root tree.
// Called at in the EventAction::EndOfEventAvtion
void Fatima::ReadSensitive(const G4Event* event){
  m_Event->Clear();

  ///////////
  // LaBr3
  G4THitsMap<G4double*>* LaBr3HitMap;
  std::map<G4int, G4double**>::iterator LaBr3_itr;

  G4int LaBr3CollectionID = G4SDManager::GetSDMpointer()->GetCollectionID("Fatima_LaBr3Scorer/FatimaLaBr3");
  LaBr3HitMap = (G4THitsMap<G4double*>*)(event->GetHCofThisEvent()->GetHC(LaBr3CollectionID));

  // Loop on the LaBr3 map
  for (LaBr3_itr = LaBr3HitMap->GetMap()->begin() ; LaBr3_itr != LaBr3HitMap->GetMap()->end() ; LaBr3_itr++){

    G4double* Info = *(LaBr3_itr->second);

    double Energy = RandGauss::shoot(Info[0], EnergyResolution);

    if(Energy>EnergyThreshold){
      double Time = Info[1];
      int DetectorNbr = (int) Info[2];

      m_Event->SetFatimaLaBr3E(DetectorNbr,Energy);
      m_Event->SetFatimaLaBr3T(DetectorNbr,Time);
    }
  }
  // clear map for next event
  LaBr3HitMap->clear();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void Fatima::InitializeScorers(){
  vector<G4int> NestingLevel;
  NestingLevel.push_back(1);

  //   LaBr3 Associate Scorer
  bool already_exist = false;
  m_LaBr3Scorer = CheckScorer("Fatima_LaBr3Scorer",already_exist);

  // if the scorer were created previously nothing else need to be made
  if(already_exist) return;

  G4VPrimitiveScorer* LaBr3Scorer =
    new  CALORIMETERSCORERS::PS_Calorimeter("FatimaLaBr3",NestingLevel);
  //and register it to the multifunctionnal detector
  m_LaBr3Scorer->RegisterPrimitive(LaBr3Scorer);

  //   Add All Scorer to the Global Scorer Manager
  G4SDManager::GetSDMpointer()->AddNewDetector(m_LaBr3Scorer) ;
}

