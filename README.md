# MuonPOG
Collection of MuonPOG related tools

## Installation instructions

cmsrel CMSSW_7_4_6_patch5 # Just an example release, works in 74X at present 

cd CMSSW_7_4_6_patch5/src/

git clone git@github.com:battibass/MuonPOG.git

cmsenv

scramv1 b -j 5

## Ntuples
Ntuple interface defined in : MuonPOG/Tools/src/MuonPogTree.h

Ntuple are filed by code in : MuonPOG/Tools/plugins/MuonPogTreeProducer.cc

To create some ntuples :

cd MuonPOG/Tools/test/

cmsRun muonPogNtuples_cfg.py # modify files according to your needs

The ntuple gets loaded by :

from MuonPOG.Tools.MuonPogNtuples_cff import appendMuonPogNtuple

appendMuonPogNtuple(process,False,"HLT","ntuple_DoubleMuon_251244_251252.root")

Where arguments are :

1. The CMS configuration process
2. A bool to say whether you are running on MC
3. The label of the process giving HLT results
4. The name of the output ntuple

## Invariant mass macro 

To run the invariant masses macro on ntuples :

cd MuonPOG/Tools/invariant_mass/

./invariantMassPlots \

/afs/cern.ch/user/b/battilan/work/public/MuonPOG_Ntuples_2015/ntuple_DoubleMuon_251244_251252.root \ 

config_z/*ini #modify input ntuple and config files according to your needs
