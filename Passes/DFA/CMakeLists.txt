add_llvm_library( submission_pt3 MODULE
  ReachingDefinitionAnalysis.cpp
  MayPointToAnalysis.cpp
  LivenessAnalysis.cpp

  PLUGIN_TOOL
  opt
  )
