/**********************************************************\
* File: QwScanner.h                                       *
*                                                         *
* Author:                                                 *
* Time-stamp:                                             *
\**********************************************************/

#ifndef __QWSCANNER__
#define __QWSCANNER__

// System headers
#include <vector>

// ROOT headers
#include "TTree.h"
#include "TString.h"
#include "TFile.h"
#include "TH2.h"
#include "TH2F.h"


// Qweak headers
#include "VQwSubsystemParity.h"
#include "QwIntegrationPMT.h"
#include "QwCombinedPMT.h"
//#include "QwScanner_Channel.h"

// add enum type for new scanner information
// create QwScannerID class based on detector array MainDetID class

class QwDetectorArrayID {

 public:

    QwDetectorArrayID():fSubbankIndex(-1),fWordInSubbank(-1),
     fTypeID(kQwUnknownPMT),fIndex(-1),
     fSubelement(kInvalidSubelementIndex),fmoduletype(""),fdetectorname("") {};

    int fSubbankIndex;
    int fWordInSubbank; //first word reported for this channel in the subbank
                        //(eg VQWK channel report 6 words for each event, scalers oly report one word per event)
                        // The first word of the subbank gets fWordInSubbank=0

    EQwPMTInstrumentType fTypeID;     // type of detector
    int fIndex;      // index of this detector in the vector containing all the detector of same type
    UInt_t fSubelement; // some detectors have many subelements (eg stripline have 4 antenas)
                        // some have only one sub element(eg lumis have one channel)

    TString fmoduletype; // eg: VQWK, SCALER
    TString fdetectorname;
    TString fdetectortype; // stripline, IntegrationPMT, ... this string is encoded by fTypeID

    std::vector<TString> fCombinedChannelNames;
    std::vector<Double_t> fWeight;

    void Print() const;

};

/*****************************************************************
*  Class: QwScanner
******************************************************************/

class QwScanner: public VQwSubsystemParity, public MQwSubsystemCloneable<QwScanner> {

  private:
    /// Private default constructor (not implemented, will throw linker error on use)
    QwScanner();

  public:

    // Constructor with name
    QwScanner(const TString& name)
      :VQwSubsystem(name),VQwSubsystemParity(name),bNormalization(kFALSE) {

        fTargetCharge.InitializeChannel("q_targ","derived");
        // define fixed voltages
        fPositionReference.first.SetHardwareSum(3);
        fPositionReference.second.SetHardwareSum(3);


    };

    // Copy constructor
    QwScanner(const QwScanner& source)
     :VQwSubsystem(source),VQwSubsystemParity(source),
     fIntegrationPMT(source.fIntegrationPMT),
     fCombinedPMT(source.fCombinedPMT),
     fMainDetID(source.fMainDetID),
     fPositionReference(source.fPositionReference) {}

    /// Destructor
    virtual ~QwScanner();

//    virtual void PublishInternalValues() override;

//    void   CopyTemplatedDataElements(const VQwSubsystem *source) override;

    // Handle command line options
    static void DefineOptions(QwOptions &options);
    void ProcessOptions(QwOptions &options);

    Int_t LoadChannelMap(TString mapfile);
    Int_t LoadInputParameters(TString pedestalfile);
    Int_t LoadMockRateMap(TString rootfilename);


    void  ClearEventData() override;

    Int_t ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words){/*Needs impementation*/ return 0;};
    Int_t ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words);
    void  ProcessEvent() override;


    // imlementation of scanner behavior
    void  SetRandomEventParameters(Double_t mean, Double_t sigma);
    void  SetRandomEventAsymmetry(Double_t asymmetry);
    void  RandomizeEventData(int helicity, Double_t time);
    void  EncodeEventData(std::vector<UInt_t> &buffer);
    void  RandomizeMollerEvent(int helicity/*, const QwBeamCharge& charge, const QwBeamPosition& xpos, const QwBeamPosition& ypos, const QwBeamAngle& xprime, const QwBeamAngle& yprime, const QwBeamEnergy& energy*/);

//    using VQwSubsystem::ConstructHistograms;
    void  ConstructHistograms(TDirectory *folder, TString &prefix) override;
    void  FillHistograms() override;

//    using VQwSubsystem::ConstructBranchAndVector;
    void  ConstructBranchAndVector(TTree *tree, TString &prefix, std::vector<Double_t> &values) override;
    void  ConstructBranch(TTree *tree, TString& prefix) { };
    void  ConstructBranch(TTree *tree, TString& prefix, QwParameterFile& trim_file) { };
    void  FillTreeVector(std::vector<Double_t> &values) const override;

    Bool_t Compare(VQwSubsystem *source);

    VQwSubsystem& operator=(VQwSubsystem *value);
    VQwSubsystem& operator+=(VQwSubsystem *value);
    VQwSubsystem& operator-=(VQwSubsystem *value);
    void SetVoltPerHz(Double_t value)     {fVoltPerHz = value;};
    void Ratio(VQwSubsystem *value1, VQwSubsystem  *value2);
    void Scale(Double_t factor);

    void AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
    //remove one entry from the running sums for devices
    void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF);
    void CalculateRunningAverage();

    const QwIntegrationPMT* GetChannel(const TString name) const;
    const QwIntegrationPMT* GetIntegrationPMT(const TString name) const;
    const QwCombinedPMT* GetCombinedPMT(const TString name) const;

//    Int_t LoadEventCuts(TString filename);
//    Bool_t SingleEventCuts();
    Bool_t ApplySingleEventCuts();

    Bool_t CheckForBurpFail(const VQwSubsystem *subsys){
        //QwError << "************* test inside scanner *****************" << QwLog::endl;
        return kFALSE;
    };

    void IncrementErrorCounters();

    void PrintErrorCounters() const;
    UInt_t GetEventcutErrorFlag();
//    //update the error flag in the subsystem level from the top level routines related to stability checks. This will uniquely update the errorflag at each channel based on the error flag in the corresponding channel in the ev_error subsystem
    void UpdateErrorFlag(const VQwSubsystem *ev_error);

    void LoadMockDataParameters(TString pedestalfile);

    void PrintValue() const;
    void PrintInfo() const;

    Double_t* GetRawChannelArray();

    Double_t GetDataForChannelInModule(Int_t modnum, Int_t channum) {
      Int_t index = fModuleChannel_Map[std::pair<Int_t,Int_t>(modnum,channum)];
      return fScanner.at(index)->GetValue();
    }

    Int_t GetChannelIndex(TString channelName, UInt_t module_number);

  private:

    // Number of good events
    Int_t fGoodEventCount;


////    // Mapping from subbank to Scanner channels
//    typedef std::map< Int_t, std::vector< std::vector<Int_t> > > Subbank_to_Scanner_Map_t;
//    Subbank_to_Scanner_Map_t fSubbank_Map;
//
////    // Mapping from module and channel number to Scanner channel
    typedef std::map< std::pair<Int_t,Int_t>, Int_t > Module_Channel_to_Scanner_Map_t;
    Module_Channel_to_Scanner_Map_t fModuleChannel_Map;
//
////    // Mapping from name to Scanner channel
//    typedef std::map< TString, Int_t> Name_to_Scanner_Map_t;
//    Name_to_Scanner_Map_t fName_Map;

    Double_t fPMTrate;
    Bool_t ldebug=kFALSE;
    Bool_t first_time = kTRUE;
    TString varname;
    Double_t varped;
    Double_t varcal;


    // mapfile variables
    Double_t varxpos;
    Double_t varypos;
    Double_t varxref;
    Double_t varyref;
    Double_t varPMT1;
    Double_t varPMT2;

    TString localname;

    Int_t lineread=0;

//    std::cout << "QwScanner is working" << std::endl;
    Double_t time_increment = fScanRate; //scan rate
//    Double_t num_ts = time / time_increment;
    Double_t old_time;
//    Double_t speed = fScanSpeed; // scanner speed
//    Double_t step = speed*time_increment; //redefine
    int num_y_pos = 0;
//    int timestep;
    Bool_t vert = false;
    Double_t bin1;
    Double_t bin2;
    Double_t bin3;
    Double_t bin4;
    Double_t x_lin_val;
    Double_t y_lin_val;
    Double_t lin_val;
    int xbin;
    int ybin;
//    Double_t x_min = fxmin; //-600;
//    Double_t y_min = fymin;
//    Double_t x_max = 600;
//    Double_t y_max = -600; // in mm
//    int x_bins = (fxmax-fxmin)/step;
//    int y_bins = (fymax-fymin)/step;
    Double_t delta_x = 1/time_increment;
//    int print_step_size = fPrintFreq;
    int print_step_size = 1000;
//    int fill_step_size = fMockSecondaryAxisStep;
    int fill_step_size = 1000;
    Double_t x_pos = fxmin;
    Double_t y_pos = fymin;
    Double_t y_goal = fymin;
    Double_t mod_x_pos;
    Double_t mod_y_pos;
    Double_t y_track_sep = 1; // in mm

    TFile *f;
    TH2F *h2;
    TH2F *h3;



    std::vector <QwIntegrationPMT*> fScanner; // Raw channels does this need star?
    std::pair <QwIntegrationPMT, QwIntegrationPMT> fXY; // Positions
    std::pair <Double_t, Double_t> fPosition; // Positions
    std::pair <QwIntegrationPMT, QwIntegrationPMT> fPositionReference; // Reference Voltages for the Position Encoders
    std::pair <Double_t, Double_t> fPositionFullScale; // width of scanning range (full range)
    std::pair <Double_t, Double_t> fPositionOrigin; // starting position for the scanner
//    std::vector <UInt_t> fBufferOffset; // Offset in scanner buffer



  protected:
    Bool_t fDEBUG;

    EQwPMTInstrumentType GetDetectorTypeID(TString name);

    Int_t GetDetectorIndex(EQwPMTInstrumentType TypeID, TString name);

    std::vector <QwIntegrationPMT> fIntegrationPMT;
    std::vector <QwCombinedPMT> fCombinedPMT;
    std::vector <QwDetectorArrayID> fMainDetID;

    QwBeamCharge   fTargetCharge;
    QwBeamPosition fTargetX;
    QwBeamPosition fTargetY;
    QwBeamAngle    fTargetXprime;
    QwBeamAngle    fTargetYprime;
    QwBeamEnergy   fTargetEnergy;

    Bool_t bIsExchangedDataValid;

    Bool_t bNormalization;
    Double_t fNormThreshold;

  private:

    static const Bool_t bDEBUG=kFALSE;
    Int_t fMainDetErrorCount;

    // Mock Parameters for rates
    // add variables from macro
    TH2F *fRateMap; //replace h2
    TFile *fFileForHist; // replace f
    Int_t fMockPrincipleScanAxis; // 1 vertical 0 horizontal
    Double_t fScanRate; // how often the scanner reads data
    Double_t fScanSpeed; // how fast the scanner moves
    Double_t fMockSecondaryAxisStep; // distance traveled on the secondary direction after completing the scan along the primary
    Double_t fScanStartTime;
    Double_t fOldTime;
    Double_t fxmin;
    Double_t fxmax;
    Double_t fymin;
    Double_t fymax;
    Double_t fTestValue;
    Double_t fPrintFreq; // how many scans are taken between data readouts
    Double_t fVoltPerHz;
};

#endif
