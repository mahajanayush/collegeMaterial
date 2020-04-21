#include "cpu/pred/gshare.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"

GShareBP::GShareBP(const GShareBPParams *params)
    : BPredUnit(params),
      globalPredictorSize(params->globalPredictorSize),
      globalCtrBits(params->globalCtrBits),
      globalCtrs(globalPredictorSize, SatCounter(globalCtrBits)),
      globalHistory(params->numThreads, 0),
      globalHistoryBits(
          ceilLog2(params->globalPredictorSize) >
          ceilLog2(params->choicePredictorSize) ?
          ceilLog2(params->globalPredictorSize) :
          ceilLog2(params->choicePredictorSize)),
      choicePredictorSize(params->choicePredictorSize),
      choiceCtrBits(params->choiceCtrBits),
      choiceCtrs(choicePredictorSize, SatCounter(choiceCtrBits))
{
    if (!isPowerOf2(globalPredictorSize)) {
        fatal("Invalid global predictor size!\n");
    }

    if (!isPowerOf2(choicePredictorSize)) {
        fatal("Invalid choice predictor size!\n");
    }

    choiceHistoryMask = choicePredictorSize - 1;

    historyRegisterMask = mask(globalHistoryBits);

    if (globalHistoryMask > historyRegisterMask) {
        fatal("Global predictor too large for global history bits!\n");
    }
    if (choiceHistoryMask > historyRegisterMask) {
        fatal("Choice predictor too large for global history bits!\n");
    }

    if (globalHistoryMask < historyRegisterMask &&
        choiceHistoryMask < historyRegisterMask) {
        inform("More global history bits than required by predictors\n");
    }

    globalThreshold = (ULL(1) << (globalCtrBits - 1)) - 1;
    choiceThreshold = (ULL(1) << (choiceCtrBits - 1)) - 1;
}


inline
void
GShareBP::updateGlobalHistTaken(ThreadID tid)
{
    globalHistory[tid] = (globalHistory[tid] << 1) | 1;
    globalHistory[tid] = globalHistory[tid] & historyRegisterMask;
}

inline
void
GShareBP::updateGlobalHistNotTaken(ThreadID tid)
{
    globalHistory[tid] = (globalHistory[tid] << 1);
    globalHistory[tid] = globalHistory[tid] & historyRegisterMask;
}


void
GShareBP::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    globalHistory[tid] &= (historyRegisterMask & ~ULL(1));
}

bool
GShareBP::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{
    bool global_prediction;
    bool choice_prediction;

    global_prediction = globalThreshold <
      globalCtrs[globalHistory[tid] & globalHistoryMask];

    choice_prediction = choiceThreshold <
      choiceCtrs[globalHistory[tid] & choiceHistoryMask];

    // Create BPHistory and pass it back to be recorded.
    BPHistory *history = new BPHistory;
    history->globalHistory = globalHistory[tid];
    history->globalPredTaken = global_prediction;
    history->globalUsed = choice_prediction;
    bp_history = (void *)history;

        if (global_prediction) {
            updateGlobalHistTaken(tid);
            return true;
        } else {
            updateGlobalHistNotTaken(tid);
            return false;
        }
}

void
GShareBP::uncondBranch(ThreadID tid, Addr pc, void * &bp_history)
{
    // Create BPHistory and pass it back to be recorded.
    BPHistory *history = new BPHistory;
    history->globalHistory = globalHistory[tid];
    history->globalPredTaken = true;
    history->globalUsed = true;
    bp_history = static_cast<void *>(history);

    updateGlobalHistTaken(tid);
}

void
GShareBP::update(ThreadID tid, Addr branch_addr, bool taken,
                     void *bp_history, bool squashed,
                     const StaticInstPtr & inst, Addr corrTarget)
{
    assert(bp_history);

    BPHistory *history = static_cast<BPHistory *>(bp_history);
    if (squashed) {
        globalHistory[tid] = (history->globalHistory << 1) | taken;
        globalHistory[tid] &= historyRegisterMask;

        return;
    }

    if (history->localPredTaken != history->globalPredTaken &&
        old_local_pred_valid)
    {
        unsigned choice_predictor_idx =
            history->globalHistory & choiceHistoryMask;
        if (history->localPredTaken == taken) {
            choiceCtrs[choice_predictor_idx]--;
        } else if (history->globalPredTaken == taken) {
            choiceCtrs[choice_predictor_idx]++;
        }
    }

    unsigned global_predictor_idx =
            history->globalHistory & globalHistoryMask;
    if (taken) {
        globalCtrs[global_predictor_idx]++;
    } else {
        globalCtrs[global_predictor_idx]--;
    }

    delete history;
}

void
GShareBP::squash(ThreadID tid, void *bp_history)
{
    BPHistory *history = static_cast<BPHistory *>(bp_history);

    globalHistory[tid] = history->globalHistory;

    delete history;
}

GShareBP*
GShareBPParams::create()
{
    return new GShareBP(this);
}

#ifdef DEBUG
int
GShareBP::BPHistory::newCount = 0;
#endif
