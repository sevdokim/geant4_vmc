// $Id: TG4PrimaryGeneratorAction.cxx,v 1.2 2003/07/22 06:36:09 brun Exp $
// Category: run
//
// Author: I. Hrivnacova
//
// Class TG4PrimaryGeneratorAction
// -------------------------------
// See the class description in the header file.

#include "TG4PrimaryGeneratorAction.h"
#include "TG4ParticlesManager.h"
#include "TG4G3Units.h"
#include "TG4Globals.h"

#include <G4Event.hh>
#include <G4ParticleTable.hh>
#include <G4ParticleDefinition.hh>

#include <TVirtualMC.h>
#include <TVirtualMCApplication.h>
#include <TVirtualMCStack.h>
#include <TParticle.h>

//_____________________________________________________________________________
TG4PrimaryGeneratorAction::TG4PrimaryGeneratorAction()
  : TG4Verbose("primaryGeneratorAction") {
//
}

//_____________________________________________________________________________
TG4PrimaryGeneratorAction::~TG4PrimaryGeneratorAction() {
//
}

// private methods

//_____________________________________________________________________________
void TG4PrimaryGeneratorAction::TransformPrimaries(G4Event* event)
{
// Creates a new G4PrimaryVertex objects for each TParticle
// in in the MC stack.
// ---
  
  TVirtualMCStack* stack = gMC->GetStack();  
  if (!stack) {
    G4String text = "TG4PrimaryGeneratorAction::TransformPrimaries:\n";
    text = text + "   No MC stack is defined.";
    TG4Globals::Exception(text);
  }  
    
  G4int nofParticles = stack->GetNtrack();
  if (nofParticles <= 0) {
    G4String text = "TG4PrimaryGeneratorAction::TransformPrimaries:\n";
    text = text + "   No primary particles found on the stack.";
    TG4Globals::Exception(text);
  }  

  if (VerboseLevel() > 1)
    G4cout << "TG4PrimaryGeneratorAction::TransformPrimaries: " 
           << nofParticles << " particles" << G4endl; 
     

  G4PrimaryVertex* previousVertex = 0;
  G4ThreeVector previousPosition = G4ThreeVector(); 
  G4double previousTime = 0.; 
  
  for (G4int i=0; i<nofParticles; i++) {    
  
    // get the particle from the stack
    TParticle* particle = stack->PopPrimaryForTracking(i);
    
    if (particle) {
      // only particles that didn't die (decay) in primary generator
      // will be transformed to G4 objects   

      // Get particle definition from TG4ParticlesManager
      //
      TG4ParticlesManager* particlesManager = TG4ParticlesManager::Instance();

      G4ParticleDefinition* particleDefinition
        = particlesManager->GetParticleDefinition(particle, false);
      G4bool isIon = false;	

      if (!particleDefinition) {
        particleDefinition
          = particlesManager->GetIonParticleDefinition(particle, false);
	if (particleDefinition) isIon = true;
      }	  	  
            
      if (!particleDefinition) {
        G4cerr << particle->GetName() << "  " 
	       << particle->GetTitle() 
	       << " pdgEncoding: " << particle->GetPdgCode() << G4endl;
        G4String text = 
            "TG4PrimaryGeneratorAction::TransformPrimaries:\n";
        text = text + "    G4ParticleTable::FindParticle() failed.";
        TG4Globals::Exception(text);
      }	

      // Get/Create vertex
      G4ThreeVector position 
        = particlesManager->GetParticlePosition(particle);
      G4double time = particle->T()*TG4G3Units::Time(); 
      G4PrimaryVertex* vertex;
      if ( i==0 || previousVertex ==0 || 
           position != previousPosition || time != previousTime ) {
        // Create a new vertex 
        // in case position and time of gun particle are different from 
        // previous values
        // (vertex objects are destroyed in G4EventManager::ProcessOneEvent()
        // when event is deleted)  
        vertex = new G4PrimaryVertex(position, time);
        event->AddPrimaryVertex(vertex);

        previousVertex = vertex;
        previousPosition = position;
        previousTime = time;
      }
      else 
        vertex = previousVertex;

      // Create a primary particle and add it to the vertex
      // (primaryParticle objects are destroyed in G4EventManager::ProcessOneEvent()
      // when event and then vertex is deleted)
      G4ThreeVector momentum 
        = particlesManager->GetParticleMomentum(particle);
      G4PrimaryParticle* primaryParticle 
        = new G4PrimaryParticle(particleDefinition, 
	                        momentum.x(), momentum.y(), momentum.z());
 
      // Set mass
      primaryParticle->SetMass(particleDefinition->GetPDGMass());

      // Set charge
      G4double charge = particleDefinition->GetPDGCharge();
      if (isIon) 
        charge = particle->GetPDG()->Charge() * eplus/3.;
      primaryParticle->SetCharge(charge);
      
      // Set polarization
      TVector3 polarization;
      particle->GetPolarisation(polarization);
      primaryParticle
        ->SetPolarization(polarization.X(), polarization.Y(), polarization.Z());  
	
      // Add primary particle to the vertex
      vertex->SetPrimary(primaryParticle);

      // Verbose
      if (VerboseLevel() > 1) {
        G4cout << i << "th primary particle: " << G4endl;
        primaryParticle->Print();
      } 
    }   
  }
}

// public methods

//_____________________________________________________________________________
void TG4PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
// Generates primary particles by the selected generator.
// ---

  // Begin of event
  TVirtualMCApplication::Instance()->BeginEvent();

  // Generate primaries and fill the MC stack
  TVirtualMCApplication::Instance()->GeneratePrimaries();
  
  // Transform Root particle objects to G4 objects
  TransformPrimaries(event);
}
