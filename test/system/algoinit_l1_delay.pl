%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Copyright: TUD/UHEI 2007 - 2012
% License: GPL
% Description: BrainScaleS Mapping Process algorithm init file
%              using GMPath [1]. 
%
% [1] K. Wendt, M. Ehrlich, and R. Schüffny, "GMPath - a path language for 
%     navigation, information query and modification of data graphs", 
%     Proceedings of ANNIIP 2010, 2010, p. 31-42
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 

EnableCreateMode

% get the global parameter node
GP = SystemNode/GlobalParameters

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Algorithmic Sequences
%
% use this section to:
%
% - set global algorithmic parameters
% - configure the algorithms
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Global Algorithm Parameters
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% The Algorithm Sequences Node
AS = GP/AlgorithmSequences/user

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% System Algorithms Configuration
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

SystemAS = AS/SystemAlgorithmSequence

% manual placement via a file containing GMPath expressions
A_PlacementExtern = SystemAS/PlacementExtern
  C = A_PlacementExtern/FileName/NeuronPlacement.mpf
  A_PlacementExtern/FileName > EQUAL > C
  C = A_PlacementExtern/InputType/2
  A_PlacementExtern/InputType > EQUAL > C

% Recursive Mapping
A_RMS = SystemAS/RecursiveMappingStep

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% System Algorithm Sequence
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

SystemAS > FOLLOWER > A_PlacementExtern > FOLLOWER > A_RMS

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Wafer Algorithms Placements
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

WaferAS = AS/WaferAlgorithmSequence

% manual placement via a file containing GMPath expressions
A_PlacementExtern = WaferAS/PlacementExtern
  C = A_PlacementExtern/FileName/NeuronPlacement.mpf
  A_PlacementExtern/FileName > EQUAL > C
  C = A_PlacementExtern/InputType/2
  A_PlacementExtern/InputType > EQUAL > C
  % Target to which to map the elements to?
  C = A_PlacementExtern/Target/AbstractNeuronSlots
  A_PlacementExtern/Target > EQUAL > C

% Hicann Out Pin Assignment
A_PlaceHicannOutPins = WaferAS/PlacementHicannOutPinAssignment
  % Target to which to map the elements to?
  C = A_PlaceHicannOutPins/Target/AbstractNeuronSlots
  A_PlaceHicannOutPins/Target > EQUAL > C

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Wafer Algorithms Routing
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Routing Layer 2 Routing 
% not taking into account L2 pulse rates
A_RouteL2 = WaferAS/RoutingL2Simple
  % randomize l1 addresses for virtual neurons
  C =  A_RouteL2/RandomL1Addresses/0
  A_RouteL2/RandomL1Addresses > EQUAL > C
  % dsitribute stimuli over available l2 connections
  C =  A_RouteL2/DistributeSources/1
  A_RouteL2/DistributeSources > EQUAL > C

% Routing Layer 1  
% supported only for hardware model : FACETSModelV2
A_RouteL1 = WaferAS/RoutingL1WaferV2
  % enable one background event generator per used L1 bus
  % can be disabled when using the ESS
  C = A_RouteL1/OneBEGPerBus/1
  A_RouteL1/OneBEGPerBus > EQUAL > C

% Checks the validity of the routing config of the SPL1 Merger
% can be run after RoutingL1WaferV2
A_RouteL1Checker = WaferAS/RoutingSPL1MergerChecker

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Wafer Algorithms Parameter Transformation
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Custom Parameter Transformation
A_ParamTrafo = WaferAS/ParameterTransformation
  % mode for transforming synaptic weights,
  % possible modes (bio->hardware)
  % {max2max,mean2mean,max2mean}
  C = A_ParamTrafo/WeightTrafoMode/max2max
  A_ParamTrafo/WeightTrafoMode > EQUAL > C
  % choose ideal transformation although
  % calibration data is available 
  C = A_ParamTrafo/IdealTransformation/0
  A_ParamTrafo/IdealTransformation > EQUAL > C
  % choose mode here auto_scale or fixed scaling
  % default is NeuronTrafoUseAutoScale<bool>(false)
  C = A_ParamTrafo/NeuronTrafoUseAutoScale/0
  A_ParamTrafo/NeuronTrafoUseAutoScale > EQUAL > C
  % parameter for NeuronTrafoUseAutoScale<bool>(true),
  % hw-range in mV between minimal V-reset and max V-thresh
  C = A_ParamTrafo/NeuronTrafoAutoScaleHWRange/200.
  A_ParamTrafo/A_ParamTrafo > EQUAL > C
  % parameter for NeuronTrafoUseAutoScale<bool>(true),
  % target reset voltage in mV
  C = A_ParamTrafo/NeuronTrafoAutoScaleTargetVReset/500.
  A_ParamTrafo/NeuronTrafoAutoScaleTargetVReset > EQUAL > C
  % parameter for NeuronTrafoUseAutoScale<bool>(false),
  % voltage shift factor in mV
  C = A_ParamTrafo/NeuronTrafoFixedScaleVShift/1200.
  A_ParamTrafo/NeuronTrafoFixedScaleVShift > EQUAL > C
  % parameter for NeuronTrafoUseAutoScale<bool>(false),
  % voltage scale factor
  C = A_ParamTrafo/NeuronTrafoFixedScaleAlphaV/10.
  A_ParamTrafo/NeuronTrafoFixedScaleAlphaV > EQUAL > C

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Wafer Algorithm Sequence
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

WaferAS > FOLLOWER > A_PlacementExtern > FOLLOWER > A_PlaceHicannOutPins > FOLLOWER > A_RouteL2 > FOLLOWER > A_RouteL1 > FOLLOWER > A_RouteL1Checker > FOLLOWER > A_ParamTrafo

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Hicann Algorithm Sequence (currently not used)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

HicannAS = AS/HicannAlgorithmSequence
