// ---------------------------------------------------------------------------- 
//  \file   MuonGenerator.cc
//  \brief  Point Sampler on the surface of a rectangular used for muon generation. 
//          Control plots of the generation variables available.
//
//  Author: Neus Lopez March <neus.lopez@ific.uv.es>
//        
//  Created: 30 Jan 2015
//
//  Copyright (c) 2015 NEXT Collaboration
// ----------------------------------------------------------------------------
#include "MuonGenerator.h"
#include "DetectorConstruction.h"
#include "BaseGeometry.h"
#include <G4GenericMessenger.hh>
#include <G4ParticleDefinition.hh>
#include <G4RunManager.hh>
#include <G4ParticleTable.hh>
#include <G4PrimaryVertex.hh>
#include <G4Event.hh>
#include <G4RandomDirection.hh>
#include <Randomize.hh>
#include <G4OpticalPhoton.hh>
#include "MuonsPointSampler.h"
#include "AddUserInfoToPV.h"

#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TMath.h>

#include "CLHEP/Units/SystemOfUnits.h"

using namespace nexus;
using namespace CLHEP;


MuonGenerator::MuonGenerator():
  G4VPrimaryGenerator(), _msg(0), _particle_definition(0),
  _energy_min(0.), _energy_max(0.), _geom(0), _momentum_X(0.),
  _momentum_Y(0.), _momentum_Z(0.)
{
  _msg = new G4GenericMessenger(this, "/Generator/MuonGenerator/",
				"Control commands of muongenerator.");
  
  G4GenericMessenger::Command& min_energy =
    _msg->DeclareProperty("min_energy", _energy_min, "Set minimum kinetic energy of the particle.");
  min_energy.SetUnitCategory("Energy");
  min_energy.SetParameterName("min_energy", false);
  min_energy.SetRange("min_energy>0.");
  
  G4GenericMessenger::Command& max_energy =
    _msg->DeclareProperty("max_energy", _energy_max, "Set maximum kinetic energy of the particle");
  max_energy.SetUnitCategory("Energy");
  
  _msg->DeclareProperty("region", _region, 
			"Set the region of the geometry where the vertex will be generated.");
  
  _msg->DeclareProperty("momentum_X", _momentum_X,"x coord of momentum");
  _msg->DeclareProperty("momentum_Y", _momentum_Y,"y coord of momentum");
  _msg->DeclareProperty("momentum_Z", _momentum_Z,"z coord of momentum");
  
  
  DetectorConstruction* detconst = (DetectorConstruction*) G4RunManager::GetRunManager()->GetUserDetectorConstruction();
  _geom = detconst->GetGeometry();
  
  muon_phi_ = new TH1F("Muon Phi distribution", "Muon distribution;Phi (ª);Entries", 400, 0., 400.); // 382.5=360+22.5
  muon_phi_reco_ = new TH1F("Muon Phi distribution reco", "Muon reco distribution;Phi (ª);Entries", 400, 0., 400.); // 382.5=360+22.5
     
  muon_theta_ = new TH1F("Muon Theta distribution", "Muon distribution;Theta (º);Entries", 90, 0., 100.);
  muon_theta_reco_ = new TH1F("Muon Theta distribution reco", "Muon reco distribution;Theta (º);Entries", 90, 0., 100.);
  
  muon_ = new TH2F("Muons", "Muons in LSC; LSC z;LSC x: Entries", 100, -1.,1., 100, -1.,1.);

}



MuonGenerator::~MuonGenerator()
{
  std::cout<<"destructor  MuonGenerator "<<std::endl;
  delete _msg;
  out_file_ = new TFile("MuonMonitor.root", "recreate");  
  muon_phi_->Write();
  muon_phi_reco_->Write();
  muon_theta_->Write();
  muon_theta_reco_->Write();
  muon_->Write();
  // TH2F *hspc = (TH2F*) muon_->DrawClone("SURF1 POL");
  // hspc->Write();
  out_file_->Close();
}

void MuonGenerator::GeneratePrimaryVertex(G4Event* event)
{
  _particle_definition = G4ParticleTable::GetParticleTable()->FindParticle("mu-");
  if (!_particle_definition)
    G4Exception("SetParticleDefinition()", "[MuonGenerator]",
                FatalException, " can not create a muon ");  

  // Generate an initial position for the particle using the geometry
  G4ThreeVector position = _geom->GenerateVertex(_region);
  // Particle generated at start-of-event
  G4double time = 0.;
  // Create a new vertex
  G4PrimaryVertex* vertex = new G4PrimaryVertex(position, time);
  // Generate uniform random energy in [E_min, E_max]
  G4double kinetic_energy = RandomEnergy();
  
  // Particle propierties
  G4double mass   = _particle_definition->GetPDGMass();
  G4double energy = kinetic_energy + mass;
  G4double pmod = std::sqrt(energy*energy - mass*mass);
  
  // Generate momentum direction in spherical coordinates
  G4double theta = GetTheta();
  G4double phi = GetPhi(); //180.+ added a Pi phase to correct the original data 
                                 //from the muon detector to the NEXT orientation 
  
  G4double x, y, z;
  
  // NEXT axis convention (z<->y) and generate with -y! towards the detector.

  // x = sin(theta*2.*TMath::Pi()/360.) * sin(phi*2.*TMath::Pi()/360.);
  // z = sin(theta*2.*TMath::Pi()/360.) * cos(phi*2.*TMath::Pi()/360.);
  // y = -cos(theta*2.*TMath::Pi()/360.);

  x = sin(theta*2.*TMath::Pi()/360.) * cos(phi*2.*TMath::Pi()/360.);
  y = cos(theta*2.*TMath::Pi()/360.); //180 - theta for the downgoing muons
  z = sin(theta*2.*TMath::Pi()/360.) * sin(phi*2.*TMath::Pi()/360.);
 
  // x = sin(theta*2.*TMath::Pi()/360.) * cos(phi*2.*TMath::Pi()/360.);
  // y = cos(TMath::Pi()-theta*2.*TMath::Pi()/360.); //180 - theta for the downgoing muons
  // z = sin(theta*2.*TMath::Pi()/360.) * sin(phi*2.*TMath::Pi()/360.+TMath::Pi()); //need of Pi phase in z for NEXT in LSC Ref. 
 
  G4ThreeVector _p_dir(x,y,z);  
  
  G4double px = pmod * _p_dir.x();
  G4double py = pmod * _p_dir.y();
  G4double pz = pmod * _p_dir.z();
  
  // std::cout<<"pmod in MuonsGeneration "<<pmod<<std::endl;
  muon_phi_->Fill(phi);  
  if (x>0. || z>0.) {
    muon_phi_reco_->Fill((TMath::ATan(-z/x))*180./TMath::Pi());
    //std::cout<<"ATan Phi MuonGenerator "<<TMath::ATan(-z/x)*180./TMath::Pi()<<std::endl;
  } 
 
  else if (x<0. || z>0.){
    muon_phi_reco_->Fill(180.-(TMath::ATan(-z/x))*180./TMath::Pi()); 
    //std::cout<<"ATan Phi MuonGenerator "<<180.-TMath::ATan(-z/x)*180./TMath::Pi()<<std::endl;
  }

  else if (x<0. || z<0.){
    muon_phi_reco_->Fill(180.+(TMath::ATan(-z/x))*180./TMath::Pi()); 
    //std::cout<<"ATan Phi MuonGenerator "<<180.+TMath::ATan(-z/x)*180./TMath::Pi()<<std::endl;
  }
  else {
    muon_phi_reco_->Fill(360.-(TMath::ATan(-z/x))*180./TMath::Pi()); 
    //std::cout<<"ATan Phi MuonGenerator "<<360.-TMath::ATan(-z/x)*180./TMath::Pi()<<std::endl;
  }
  
  muon_theta_->Fill(theta);  
  muon_theta_reco_->Fill((TMath::ATan(std::sqrt(z*z+x*x)/y))*180./TMath::Pi());    
  //std::cout<<"ATan Phi MuonGenerator "<<TMath::ATan(std::sqrt(z*z+x*x)/y)*180./TMath::Pi()<<std::endl;

  // std::cout<<"(x,y,z) MuonGenerator "<<x<<" "<<y<<" "<<z<<" "<<std::endl;
  muon_->Fill(z,x);

  // If user provides a momentum direction, this one is used
  if (_momentum_X != 0. || _momentum_Y != 0. || _momentum_Z != 0.) {
    // Normalize if needed
    G4double mom_mod = std::sqrt(_momentum_X * _momentum_X +
				 _momentum_Y * _momentum_Y +
				 _momentum_Z * _momentum_Z);
    px = pmod * _momentum_X/mom_mod;
    py = pmod * _momentum_Y/mom_mod;
    pz = pmod * _momentum_Z/mom_mod;
  }
  
  // Create the new primary particle and set it some properties
  G4PrimaryParticle* particle = 
    new G4PrimaryParticle(_particle_definition, px, py, pz);
  
  // Set random polarization
  if (_particle_definition == G4OpticalPhoton::Definition()) {
    G4ThreeVector polarization = G4RandomDirection();
    particle->SetPolarization(polarization);
  }
  
  // Add info to PrimaryVertex to be accessed from EventAction type class to make histos of variables generated here.
  AddUserInfoToPV *info = new AddUserInfoToPV(theta, phi);
  
  vertex->SetUserInformation(info);
  
  // Add particle to the vertex and this to the event
  vertex->SetPrimary(particle);
  event->AddPrimaryVertex(vertex);
  
}

G4double MuonGenerator::RandomEnergy() const
{
  if (_energy_max == _energy_min) 
    return _energy_min;
  else
    return (G4UniformRand()*(_energy_max - _energy_min) + _energy_min);
}


G4double MuonGenerator::GetTheta() 
{
  //From LSC muon flux measurements, rebined probabilities to 3 bin (0-30,30-60,60-90)º
  //Could be generalized for new measurements :
  G4double rand = G4UniformRand();
  G4double rnd = 30.*G4UniformRand();
  if (rand < 0.2874){ //0-30
    return (rnd);
  }
  else  if (rand < 0.2874+0.6048){ //30-60
    return (30. + rnd);
  }
  else {//60-90
    return (60. + rnd);
  } 
}

G4double MuonGenerator::GetPhi() 
{
  //From LSC muon flux measurements, rebined probabilities to 360º/8  (bins centered in 0,45,90,135,180,225,270,315)
  //Could be generalized for new measurements :
  G4double rand = G4UniformRand();
  G4double rnd = 45.*G4UniformRand();
  if (rand < 0.05){ //45
    return (22.5+rnd);
  }
  else  if (rand < 0.05+0.0875){ //90
    return (67.5 + rnd);
  }
  else  if (rand < 0.05+0.0875+0.225){ //135
    return (112.5 + rnd);
  }
  else  if (rand < 0.05+0.0875+0.225+0.24375){ //180
    return (157.5 + rnd);
  }
 else  if (rand < 0.05+0.0875+0.225+0.24375+0.1625){ //225
    return (202.5 + rnd);
  }
  else  if (rand < 0.05+0.0875+0.225+0.24375+0.1625+0.08125){ //270
    return (247.5 + rnd);
  }
  else  if (rand < 0.05+0.0875+0.225+0.24375+0.1625+0.08125+0.0625){ //315
    return (292.5 + rnd);
  }
  else { //360==0
    return (337.5 + rnd);
  }

}


