// ----------------------------------------------------------------------------
// nexus | SensMap.cc
//
// This class is the primary generator of a number of back-to-back gammas
// in a position read by a list generated by the FullRingInfinity geometry.
// It is used to produce sensitivity maps.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#include "SensMap.h"

#include "DetectorConstruction.h"
#include "nexus/GeometryBase.h"

#include <G4GenericMessenger.hh>
#include <G4ParticleDefinition.hh>
#include <G4RunManager.hh>
#include <G4ParticleTable.hh>
#include <G4PrimaryVertex.hh>
#include <G4Event.hh>
#include <G4RandomDirection.hh>
#include <Randomize.hh>
#include <G4OpticalPhoton.hh>

#include "CLHEP/Units/SystemOfUnits.h"

using namespace nexus;
using namespace CLHEP;



SensMap::SensMap():
  G4VPrimaryGenerator(), msg_(0), num_gammas_(1)
{
  msg_ = new G4GenericMessenger(this, "/Generator/SensMap/",
    "Control commands of the sensitivity map primary generator.");

    msg_->DeclareProperty("num_gammas", num_gammas_,
			  "Set number of back-to-back gammas to be generated.");

  // Retrieve pointer to detector geometry from the run manager
  DetectorConstruction* detconst =
    (DetectorConstruction*) G4RunManager::GetRunManager()->GetUserDetectorConstruction();
  geom_ = detconst->GetGeometry();
}


SensMap::~SensMap()
{
  delete msg_;
}


void SensMap::GeneratePrimaryVertex(G4Event* event)
{
  // Select an initial position for the gammas using the geometry
  G4ThreeVector position = geom_->GenerateVertex("SENSITIVITY");
  // Gammas generated at start-of-event
  G4double time = 0.;
  // Create a new vertex
  G4PrimaryVertex* vertex = new G4PrimaryVertex(position, time);


  G4ParticleDefinition* particle_definition =
    G4ParticleTable::GetParticleTable()->FindParticle("gamma");
  // Set masses to PDG values
  G4double mass = particle_definition->GetPDGMass();
  // Set charges to PDG value
  G4double charge = particle_definition->GetPDGCharge();

  // Calculate cartesian components of momentum
  G4double energy = 510.999*keV + mass;
  G4double pmod = std::sqrt(energy*energy - mass*mass);

  // Add as many gamma pairs to the vertex as requested by the user
  for (G4int i=0; i<num_gammas_; i++) {

    G4ThreeVector momentum_direction = G4RandomDirection();
    G4double px = pmod * momentum_direction.x();
    G4double py = pmod * momentum_direction.y();
    G4double pz = pmod * momentum_direction.z();

    G4PrimaryParticle* particle1 =
      new G4PrimaryParticle(particle_definition, px, py, pz);
    particle1->SetMass(mass);
    particle1->SetCharge(charge);
    particle1->SetPolarization(0.,0.,0.);
    vertex->SetPrimary(particle1);

    G4PrimaryParticle* particle2 =
      new G4PrimaryParticle(particle_definition, -px, -py, -pz);
    particle2->SetMass(mass);
    particle2->SetCharge(charge);
    particle2->SetPolarization(0.,0.,0.);
    vertex->SetPrimary(particle2);

  }

  // Add vertex to the event
  event->AddPrimaryVertex(vertex);
}
