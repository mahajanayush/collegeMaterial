#ifndef __CPU_PRED_GSHARE_PRED_HH__
#define __CPU_PRED_GSHARE_PRED_HH__

#include <vector>

#include "base/sat_counter.hh"
#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/GShareBP.hh"

class GShareBP : public BPredUnit
{
  public:
    GShareBP(const GShareBPParams *params);
    bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);

    void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);
    void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);
     
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst, Addr corrTarget);
    void squash(ThreadID tid, void *bp_history);

  private:
    inline bool getPrediction(uint8_t &count);

    inline void updateGlobalHistTaken(ThreadID tid);

    inline void updateGlobalHistNotTaken(ThreadID tid);

    struct BPHistory {
#ifdef DEBUG
        BPHistory()
        { newCount++; }
        ~BPHistory()
        { newCount--; }

        static int newCount;
#endif
        unsigned globalHistory;
        bool globalPredTaken;
        bool globalUsed;
    };

    static const int invalidPredictorIndex = -1;

    unsigned globalPredictorSize;

    unsigned globalCtrBits;

    std::vector<SatCounter> globalCtrs;

    std::vector<unsigned> globalHistory;

    unsigned globalHistoryBits;

    unsigned globalHistoryMask;

    unsigned choiceHistoryMask;

    unsigned historyRegisterMask;

    unsigned choicePredictorSize;

    unsigned choiceCtrBits;

    std::vector<SatCounter> choiceCtrs;

    unsigned globalThreshold;
    unsigned choiceThreshold;
};

#endif // __CPU_PRED_TOURNAMENT_PRED_HH__
