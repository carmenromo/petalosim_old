// ----------------------------------------------------------------------------
// petalosim | PetSensorsEventAction.h
//
// This is the default event action of the PETALO simulations. Only events with
// deposited energy larger than 0 are saved in the nexus output file.
//
// The PETALO Collaboration
// ----------------------------------------------------------------------------

#ifndef PET_SENSORS_EVENT_ACTION_H
#define PET_SENSORS_EVENT_ACTION_H

#include <G4UserEventAction.hh>
#include <globals.hh>

class G4Event;
class G4GenericMessenger;

/// This class is a general-purpose event run action.

class PetSensorsEventAction : public G4UserEventAction
{
public:
  /// Constructor
  PetSensorsEventAction();
  /// Destructor
  ~PetSensorsEventAction();

  /// Hook at the beginning of the event loop
  void BeginOfEventAction(const G4Event *);
  /// Hook at the end of the event loop
  void EndOfEventAction(const G4Event *);

private:
  G4GenericMessenger *msg_;
  G4int nevt_, nupdate_;
  G4double energy_threshold_;
  G4double energy_max_;
};

#endif
