// ----------------------------------------------------------------------------
//  $Id$
//
//  Author:  <justo.martin-albo@ific.uv.es>
//  Created: 25 Dec 2010
//
//  Copyright (c) 2010-2012 NEXT Collaboration. All rights reserved.
// ----------------------------------------------------------------------------

#include "XenonLiquidProperties.h"

#include <G4AnalyticalPolSolver.hh>
#include <G4MaterialPropertiesTable.hh>

#include "CLHEP/Units/SystemOfUnits.h"
#include "CLHEP/Units/PhysicalConstants.h"


namespace nexus {

  using namespace CLHEP; 
  
  XenonLiquidProperties::XenonLiquidProperties():
    _pressure(1.*bar), _temperature(160.*kelvin)
  {
  }
  
  
  
  XenonLiquidProperties::~XenonLiquidProperties()
  {
  }
  

  
  G4double XenonLiquidProperties::Density()
  {

    // Density at 1 atm, T ~ 160 K
    _density = 2.953 * g/cm3;

    return _density;
  }
  

  
  G4double XenonLiquidProperties::RefractiveIndex(G4double energy)
  {
    // Formula for the refractive index taken from
    // A. Baldini et al., "Liquid Xe scintillation calorimetry 
    // and Xe optical properties", arXiv:physics/0401072v1 [physics.ins-det]
    
    // The Lorentz-Lorenz equation (also known as Clausius-Mossotti equation)
    // relates the refractive index of a fluid with its density:
    // (n^2 - 1) / (n^2 + 2) = - A · d_M,     (1)
    // where n is the refractive index, d_M is the molar density and
    // A is the first refractivity viral coefficient:
    // A(E) = \sum_i^3 P_i / (E^2 - E_i^2),   (2)
    // with:
    G4double P[3] = {71.23, 77.75, 1384.89}; // [eV^3 cm3 / mole]
    G4double E[3] = {8.4, 8.81, 13.2};       // [eV]

    // Note.- Equation (1) has, actually, a sign difference with respect 
    // to the one appearing in the reference. Otherwise, it yields values
    // for the refractive index below 1.

    // Let's calculate the virial coefficient.
    // We won't use the implicit system of units of Geant4 because
    // it results in loss of numerical precision.

    energy = energy / eV;
    G4double virial = 0.;

    for (G4int i=0; i<3; i++)
      virial = virial + P[i] / (energy*energy - E[i]*E[i]);
    
    G4double mol_density = Density() / 131.29;
    G4double alpha = virial * mol_density;
    
    // Isolating now the n2 from equation (1) and taking the square root
    G4double n2 = (1. - 2*alpha) / (1. + alpha);

    if (n2 < 1.) {
 //      G4String msg = "Non-physical refractive index for energy "
	// + bhep::to_string(energy) + " eV. Use n=1 instead.";
 //      G4Exception("[XenonLiquidProperties]", "RefractiveIndex()",
	// 	  JustWarning, msg);
      n2 = 1.;
    }
    
    return sqrt(n2);
  }
  

  
  G4double XenonLiquidProperties::Scintillation(G4double energy)
  {
     // A. Baldini et al., "Liquid Xe scintillation calorimetry 
    // and Xe optical properties", arXiv:physics/0401072v1 [physics.ins-det]
    // Everybody cites Jirtner, Meyer,Rice, Wilson, J. Chem. Phys. 42, 4250 (1965)
    G4double Wavelength_peak = 178*nm; 
    G4double Wavelength_FWHM = 14.*nm;
    G4double Wavelength_sigma = Wavelength_FWHM/2.35;

    G4double Energy_peak = (h_Planck*c_light/Wavelength_peak);
    G4double Energy_sigma = (h_Planck*c_light*Wavelength_sigma/pow(Wavelength_peak,2));
    // G4double bin = 6*Energy_sigma/500;

    G4double intensity = 
	  exp(-pow(Energy_peak/eV-energy/eV,2)/(2*pow(Energy_sigma/eV, 2)))/(Energy_sigma/eV*sqrt(pi*2.));

    return intensity;
  }


  
  void XenonLiquidProperties::Scintillation
  (G4int entries, G4double* energy, G4double* intensity)
  {
    for (G4int i=0; i<entries; i++)
      intensity[i] = Scintillation(energy[i]);
  }
  
  
 
  
} // end namespace nexus