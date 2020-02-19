add_llvm_library( submission_pt2 MODULE
  ReachingDefinitionAnalysis.cpp
  MayPointToAnalysis.cpp
  LivenessAnalysis.cpp

  PLUGIN_TOOL
  opt
  )
