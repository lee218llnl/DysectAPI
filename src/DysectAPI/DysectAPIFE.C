#include "DysectAPI/DysectAPIFE.h" 
#include "STAT_FrontEnd.h"

#include "libDysectAPI/include/DysectAPI/Frontend.h"

using namespace std;
using namespace DysectAPI;

// XXX: Refactoring work: move logic to API

FE::FE(const char* libPath, STAT_FrontEnd* fe, int timeout) : controlStream(0) {
  assert(fe != 0);
  assert(libPath != 0);

  sessionLib = new SessionLibrary(libPath);

  if(!sessionLib->isLoaded()) {
    loaded = false;
    return ;
  }

  if(timeout < 0) {
    // No timeout
    // Use 'enter-to-break'
    Frontend::setStopCondition(true, false, 0);

  } else {
    // Timeout only
    Frontend::setStopCondition(false, true, timeout);
  }

  //
  // Map front-end related library methods
  //

  DysectAPI::defaultProbes();

  if(sessionLib->mapMethod<proc_start_t>("on_proc_start", &lib_proc_start) != OK) {
    loaded = false;
    return ;
  }

  struct DysectFEContext_t context;
  context.network = fe->network_;
  context.processTable = fe->proctab_;
  context.processTableSize = fe->nApplProcs_;
  context.mrnetRankToMpiRanksMap = &(fe->mrnetRankToMpiRanksMap_);
  context.statFE = fe;

  // Setup session
  lib_proc_start();

  statFE = fe;
  network = fe->network_;
  filterPath = fe->filterPath_;

  int upstreamFilterId = network->load_FilterFunc(filterPath, "dysectAPIUpStream");
  if (upstreamFilterId == -1)
  {
    fprintf(stderr, "Frontend: up-stream filter not loaded\n");
    loaded = false;
    return ;
  }

  context.upstreamFilterId = upstreamFilterId;

  Err::verbose(true, "Creating streams...");
  if(Frontend::createStreams(&context) != OK) {
    loaded = false;
    return ;
  }

  loaded = true;
}

FE::~FE() {
  if(sessionLib) {
    delete(sessionLib);
  }
}

DysectErrorCode FE::requestBackendSetup(const char *libPath) {

  struct timeval start, end;
  gettimeofday(&start, NULL);

  MRN::Communicator* broadcastCommunicator = network->get_BroadcastCommunicator();

  controlStream = network->new_Stream(broadcastCommunicator, MRN::TFILTER_SUM, MRN::SFILTER_WAITFORALL);
  if(!controlStream) {
    return Error;
  }

  // Check for pending acks
  int ret = statFE->receiveAck(true);
  if (ret != STAT_OK) {
    return Error;
  }

  //
  // Send library path to all backends
  //
  if (controlStream->send(PROT_LOAD_SESSION_LIB, "%s", libPath) == -1) {
    return Error;
  }

  if (controlStream->flush() == -1) {
    return Error;
  }

  //
  // Block and wait for all backends to acknowledge
  //
  int tag;
  MRN::PacketPtr packet;
  
  Err::verbose(true, "Block and wait for all backends to confirm library load...");
  if(controlStream->recv(&tag, packet, true) == -1) {
    return Error;
  }

  int numIllBackends = 0;
  //
  // Unpack number of ill backends
  //
  if(packet->unpack("%d", &numIllBackends) == -1) {
    return Error;
  }

  if(numIllBackends != 0) {
    fprintf(stderr, "Frontend: %d backends reported error while loading session library '%s'\n", numIllBackends, libPath);
    return Error;
  }

  //
  // Broadcast request for notification upon finishing
  // stream binding.
  // Do not wait for response until all init packets have been broadcasted.
  //
  Err::verbose(true, "Broadcast request for notification upon finishing stream binding");
  if(controlStream->send(DysectGlobalReadyTag, "") == -1) {
    return Error;
  }

  if(controlStream->flush() == -1) {
    return Error;
  }

  //
  // Broadcast init packets on all created streams
  //
  Err::verbose(true, "Broadcast init packets on all created streams");
  if(Frontend::broadcastStreamInits() != OK) {
    return Error;
  }

  Err::verbose(true, "Waiting for backends to finish stream binding...");
  if(controlStream->recv(&tag, packet, true) == -1) {
    return Error;
  }

  //
  // Unpack number of ill backends
  //
  if (packet->unpack("%d", &numIllBackends) == -1) {
    return Error;
  }

  if (numIllBackends != 0) {
    fprintf(stderr, "Frontend: %d backends reported error while bindings streams\n", numIllBackends);
    return Error;
  }
  
  //
  // Kick off session
  //
  Err::verbose(true, "Kick off session");
  if (controlStream->send(DysectGlobalStartTag, "") == -1) {
    return Error;
  }

  if (controlStream->flush() == -1) {
    return Error;
  }

  gettimeofday(&end, NULL);

  // Bring both times into milliseconds

  long startms = ((start.tv_sec) * 1000) + ((start.tv_usec) / 1000);
  long endms = ((end.tv_sec) * 1000) + ((end.tv_usec) / 1000);

  long elapsedms = endms - startms;
  
  Err::info(true, "DysectAPI setup took %ld ms", elapsedms);
  
  return OK;
}

DysectErrorCode FE::requestBackendShutdown() {
  return OK;
}

bool FE::isLoaded() {
  return loaded;
}

DysectErrorCode FE::handleEvents() {
  DysectErrorCode result = Error;
  if(Frontend::listen() == OK) {
    result = OK;
  }

  return result;
}
