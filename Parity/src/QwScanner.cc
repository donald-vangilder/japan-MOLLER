//
// C++ Implementation: QwScanner
//
// Description: used to collect and process the information from the Scanner channel

#include "QwScanner.h"

// Qweak headers
#include "QwParameterFile.h"

// Register this subsystem with the factory
RegisterSubsystemFactory(QwScanner);


void QwScanner::DefineOptions(QwOptions &options)
{
  // Define command line options
}

void QwScanner::ProcessOptions(QwOptions &options)
{
  // Handle command line options
}


//Constructor
//QwScanner::QwScanner(const TString& name)
//: VQwSubsystem(name), VQwSubsystemParity(name), fTestValue(0.0), fGoodEventCount(0)
//{
////  LoadMockRateMap("us_test_XY_r_EG1_65.root");
//}

//Destructor (Delete histograms and clean up Scanner channel objects)

QwScanner::~QwScanner()
{
  // Delete Scanners
  for (size_t i = 0; i < fScanner.size(); i++) {
    delete fScanner.at(i);
  }
  fScanner.clear();
}

//void QwScanner::PublishInternalValues()
//{
//  for (auto* chan : fScanner) {
//    chan->PublishInternalValues();
//  }
//}

//void QwScanner::CopyTemplatedDataElements(const VQwSubsystem* source)
//{
//  const QwScanner* input = dynamic_cast<const QwScanner*>(source);
//  if (!input) return;
//  fTestValue = input->fTestValue;
//  fGoodEventCount = input->fGoodEventCount;
//}

//***********************************************************************************************************//
Int_t QwScanner::LoadChannelMap(TString mapfile) {

    Bool_t ldebug=kFALSE;

    std::vector<TString> combinedchannelnames;
    std::vector<Double_t> weight;
    Int_t wordsofar=0;
    Int_t currentsubbankindex=-1;
    Int_t sample_size=0;
    Double_t abs_saturation_limit = 8.5; // default saturation limit(volt)
    Bool_t bAssignedLimit = kFALSE;

    // Open the file
    QwParameterFile mapstr(mapfile.Data());
    TString varname, varvalue;

    fDetectorMaps.insert(mapstr.GetParamFileNameContents());
    mapstr.EnableGreediness();
    mapstr.SetCommentChars("!");

    UInt_t value;
    size_t vqwk_buffer_offset = 0;

    while (mapstr.ReadNextLine()) {

        RegisterRocBankMarker(mapstr);
        if (mapstr.PopValue("abs_saturation_limit",value)) {
                abs_saturation_limit=value;
                bAssignedLimit = kTRUE;
        }

        if (mapstr.PopValue("sample_size",value)) {
                sample_size=value;
        }

        if (mapstr.PopValue("vqwk_buffer_offset",value)) {
            vqwk_buffer_offset=value;
        }

        mapstr.TrimComment('!');   // Remove everything after a '!' character.
        mapstr.TrimWhitespace();   // Get rid of leading and trailing spaces.

        if (mapstr.LineIsEmpty())  continue;

        Bool_t  lineok   = kTRUE;
            TString keyword  = "";
            TString keyword2 = "";
        TString modtype  = "";
        TString dettype  = "";
            TString namech   = "";
        Int_t modnum     = 0;
            Int_t channum    = 0;

            modtype = mapstr.GetTypedNextToken<TString>();      // module type

        modtype.ToUpper();

        if (modtype == "VPMT") {

            channum       = mapstr.GetTypedNextToken<Int_t>();  //channel number
            Int_t combinedchans = mapstr.GetTypedNextToken<Int_t>();    //number of combined channels
            dettype     = mapstr.GetTypedNextToken<TString>();  //type-purpose of the detector
            dettype.ToLower();
            namech      = mapstr.GetTypedNextToken<TString>();  //name of the detector
            namech.ToLower();
            combinedchannelnames.clear();

            for (int i=0; i<combinedchans; i++){

                TString nameofcombinedchan = mapstr.GetTypedNextToken<TString>();
                nameofcombinedchan.ToLower();
                combinedchannelnames.push_back(nameofcombinedchan);
            }

            weight.clear();

            for (int i=0; i<combinedchans; i++) {

                weight.push_back( mapstr.GetTypedNextToken<Double_t>());
            }

            keyword  = mapstr.GetTypedNextToken<TString>();
                keyword.ToLower();
                keyword2 = mapstr.GetTypedNextToken<TString>();
                keyword2.ToLower();

        } else {

            modnum    = mapstr.GetTypedNextToken<Int_t>();      //slot number
            channum   = mapstr.GetTypedNextToken<Int_t>();      //channel number
            dettype = mapstr.GetTypedNextToken<TString>();      //type-purpose of the detector
            dettype.ToLower();
            namech  = mapstr.GetTypedNextToken<TString>();  //name of the detector
            namech.ToLower();

                keyword   = mapstr.GetTypedNextToken<TString>();
                keyword.ToLower();
                keyword2  = mapstr.GetTypedNextToken<TString>();
                keyword2.ToLower();
        }


        if (currentsubbankindex!=GetSubbankIndex(fCurrentROC_ID,fCurrentBank_ID)) {

            currentsubbankindex=GetSubbankIndex(fCurrentROC_ID,fCurrentBank_ID);
            wordsofar=0;
        }

        QwDetectorArrayID localMainDetID;
        localMainDetID.fdetectorname=namech;
        localMainDetID.fmoduletype=modtype;
        localMainDetID.fSubbankIndex=currentsubbankindex;
        localMainDetID.fdetectortype=dettype;

            //localMainDetID.fWordInSubbank=wordsofar;

        if (modtype=="MOLLERADC") {

                Int_t offset = QwMollerADC_Channel::GetBufferOffset(modnum, channum)+vqwk_buffer_offset;

                if (offset>=0){

                    localMainDetID.fWordInSubbank = wordsofar + offset;
                }

            } else if (modtype=="VPMT") {

            localMainDetID.fCombinedChannelNames = combinedchannelnames;
            localMainDetID.fWeight = weight;

            //std::cout<<"Add in a combined channel"<<std::endl;
        } else {

            QwError << "QwScanner::LoadChannelMap:  Unknown module type: "
                     << modtype <<", "<<namech<<" will not be decoded "
                     << QwLog::endl;
            lineok=kFALSE;
            continue;
        }

        localMainDetID.fTypeID=GetDetectorTypeID(dettype);

            if (localMainDetID.fTypeID==kQwUnknownPMT) {
		if ((dettype == "position") || (dettype == "posref")) {
			// to be done later
			continue;
		}

		else {

	                QwError << "QwScanner::LoadChannelMap:  Unknown detector type: "
	                     << dettype <<", the detector "<<namech<<" will not be decoded "
	                     << QwLog::endl;
	                lineok=kFALSE;
	                continue;
		}
            }

        localMainDetID.fIndex= GetDetectorIndex(localMainDetID.fTypeID,
        localMainDetID.fdetectorname);

        if (localMainDetID.fIndex==-1){

            if (localMainDetID.fTypeID==kQwIntegrationPMT){

                QwIntegrationPMT localIntegrationPMT(GetName(),localMainDetID.fdetectorname);

                        if (keyword=="not_blindable" || keyword2=="not_blindable")
                         localIntegrationPMT.SetBlindability(kFALSE);

                        else
                         localIntegrationPMT.SetBlindability(kTRUE);

                        if (keyword=="not_normalizable" || keyword2=="not_normalizable")
                             localIntegrationPMT.SetNormalizability(kFALSE);

                        else
                             localIntegrationPMT.SetNormalizability(kTRUE);

                        fIntegrationPMT.push_back(localIntegrationPMT);
                fIntegrationPMT[fIntegrationPMT.size()-1].SetDefaultSampleSize(sample_size);

                        if(bAssignedLimit)
                         fIntegrationPMT[fIntegrationPMT.size()-1].SetSaturationLimit(abs_saturation_limit);

                        localMainDetID.fIndex=fIntegrationPMT.size()-1;

            } else if (localMainDetID.fTypeID==kQwCombinedPMT) {

                        QwCombinedPMT localcombinedPMT(GetName(),localMainDetID.fdetectorname);

                        if (keyword=="not_normalizable" || keyword2=="not_normalizable")
                         localcombinedPMT.SetNormalizability(kFALSE);

                        else
                         localcombinedPMT.SetNormalizability(kTRUE);

                        if (keyword=="not_blindable" || keyword2 =="not_blindable")
                         localcombinedPMT.SetBlindability(kFALSE);

                        else
                         localcombinedPMT.SetBlindability(kTRUE);

                fCombinedPMT.push_back(localcombinedPMT);
                fCombinedPMT[fCombinedPMT.size()-1].SetDefaultSampleSize(sample_size);
                localMainDetID.fIndex=fCombinedPMT.size()-1;
            }
        }

        if (ldebug) {

            localMainDetID.Print();
            std::cout<<"line ok=";

            if (lineok)
             std::cout<<"TRUE"<<std::endl;

            else
             std::cout<<"FALSE"<<std::endl;
        }

        if (lineok)
         fMainDetID.push_back(localMainDetID);

    } // End of "while (mapstr.ReadNextLine())"

    for (size_t i=0; i<fMainDetID.size(); i++) {

        if (fMainDetID[i].fTypeID==kQwCombinedPMT) {

            Int_t ind = fMainDetID[i].fIndex;

            //check to see if all required channels are available
            if (ldebug) {

                std::cout<<"fMainDetID[i].fCombinedChannelNames.size()="
                 <<fMainDetID[i].fCombinedChannelNames.size()<<std::endl<<"name list: ";

                for (size_t n=0; n<fMainDetID[i].fCombinedChannelNames.size(); n++)
                 std::cout<<"  "<<fMainDetID[i].fCombinedChannelNames[n];

                std::cout<<std::endl;
            }

            Int_t chanmatched=0;

            for (size_t j=0; j<fMainDetID[i].fCombinedChannelNames.size(); j++) {

                for (size_t k=0; k<fMainDetID.size(); k++) {

                    if (fMainDetID[i].fCombinedChannelNames[j]==fMainDetID[k].fdetectorname) {

                        if (ldebug)
                         std::cout<<"found a to-be-combined channel candidate"<<std::endl;

                        chanmatched ++;
                        break;
                    }
                }
            }

            if ((Int_t) fMainDetID[i].fCombinedChannelNames.size()==chanmatched) {

                for (size_t l=0; l<fMainDetID[i].fCombinedChannelNames.size(); l++) {

                    Int_t ind_pmt = GetDetectorIndex(GetDetectorTypeID("integrationpmt"),
                    fMainDetID[i].fCombinedChannelNames[l]);

                    fCombinedPMT[ind].Add(&fIntegrationPMT[ind_pmt],fMainDetID[i].fWeight[l]);
                }

                fCombinedPMT[ind].LinkChannel(fMainDetID[i].fdetectorname);

                if (ldebug)
                 std::cout<<"linked a combined channel"<<std::endl;
            } else {

                std::cerr<<"cannot combine void channels for "<<fMainDetID[i].fdetectorname<<std::endl;
                fMainDetID[i].fIndex = -1;
                continue;
            }
        }
    }


     // Now load the variables to publish
    mapstr.RewindToFileStart();
    QwParameterFile *section;
    std::vector<TString> publishinfo;
    while ((section = mapstr.ReadNextSection(varvalue))) {

        if (varvalue == "PUBLISH") {

            fPublishList.clear();

            while (section->ReadNextLine()) {

                section->TrimComment(); // Remove everything after a comment character
                section->TrimWhitespace(); // Get rid of leading and trailing spaces

                for (int ii = 0; ii < 4; ii++) {

                    varvalue = section->GetNextToken().c_str();

                    if (varvalue.Length()) {

                        publishinfo.push_back(varvalue);
                    }
                }

                if (publishinfo.size() == 4)
                 fPublishList.push_back(publishinfo);

                publishinfo.clear();
            }
        }
    }

    // Print list of variables to publish
    if (fPublishList.size()>0){

        QwMessage << "Variables to publish:" << QwLog::endl;

        for (size_t jj = 0; jj < fPublishList.size(); jj++)
         QwMessage << fPublishList.at(jj).at(0) << " " << fPublishList.at(jj).at(1) << " "
                  << fPublishList.at(jj).at(2) << " " << fPublishList.at(jj).at(3) << QwLog::endl;
    }

    if (ldebug) {

        std::cout<<"Done with Load channel map\n";

        for (size_t i=0;i<fMainDetID.size();i++)
         if (fMainDetID[i].fIndex>=0)
          fMainDetID[i].Print();
    }

    ldebug=kFALSE;
    mapstr.Close(); // Close the file (ifstream)

    return 0;
}

//***********************************************************************************************************//
Int_t QwScanner::LoadInputParameters(TString pedestalfile)
{

    Bool_t ldebug=kTRUE;
    TString varname;
    Double_t varped;
    Double_t varcal;

    //  Double_t varbaserate;
    Double_t varscanrate;
    Double_t varscanspeed;
    Double_t varsecaxisstep;
    Double_t varxmin;
    Double_t varxmax;
    Double_t varymin;
    Double_t varymax;

    TString localname;

    Int_t lineread=0;

    QwParameterFile mapstr(pedestalfile.Data());  //Open the file
    fDetectorMaps.insert(mapstr.GetParamFileNameContents());

    while (mapstr.ReadNextLine()) {

        lineread+=1;
        if (ldebug)std::cout<<" line read so far ="<<lineread<<"\n";
         mapstr.TrimComment('!');   // Remove everything after a '!' character.

        mapstr.TrimWhitespace();   // Get rid of leading and trailing spaces.

        if (mapstr.LineIsEmpty())  continue;

        else {
            varname = mapstr.GetTypedNextToken<TString>();      //name of the channel
            varname.ToLower();
            varname.Remove(TString::kBoth,' ');
            varscanrate    = mapstr.GetTypedNextToken<Double_t>();
            varscanspeed   = mapstr.GetTypedNextToken<Double_t>();
            varsecaxisstep = mapstr.GetTypedNextToken<Double_t>();
            varxmin        = mapstr.GetTypedNextToken<Double_t>();
            varxmax        = mapstr.GetTypedNextToken<Double_t>();
            varymin        = mapstr.GetTypedNextToken<Double_t>();
            varymax        = mapstr.GetTypedNextToken<Double_t>();

            if (ldebug)
             std::cout << "Inputs for channel "      << varname        << ": ped="  << varped  << ": cal=" << varcal << "\n"
                       << ": Scan Rate="             << varscanrate    << "\n"
                       << ": Scan Speed="            << varscanspeed   << "\n"
                       << ": Secondary Axis Step="   << varsecaxisstep << "\n"
                       << ": Xmin=" << varxmin << ", Xmax=" << varxmax << "\n"
                       << ": Ymin=" << varymin << ", Ymax=" << varymax << "\n";

            // Bool_t notfound=kTRUE;

            // if (notfound)
            for (size_t i=0;i<fIntegrationPMT.size();i++){
              if (fIntegrationPMT[i].GetElementName()==varname) {

                fIntegrationPMT[i].SetPedestal(varped);
                fIntegrationPMT[i].SetCalibrationFactor(varcal);

                break;

              }
            }
        }
    }


    f = TFile::Open("../us_test_XY_r_EG1_65.root");

    h2 = (TH2F*)f->Get("det175/det175_XY_evt_epiM_rate_EG1");
    if (!h2) {
        std::cerr << "Error: histogram not found" << std::endl;
        f->ls();
    }
    h3 = (TH2F*)h2->Clone();

    if (ldebug)
     std::cout<<" line read in the pedestal + cal file ="<<lineread<<" \n";

    ldebug=kFALSE;
    mapstr.Close(); // Close the file (ifstream)


//    Bool_t ldebug=kFALSE;
//
//    TString varname;
//    Int_t lineread=1;
//
//    if(ldebug)std::cout<<"QwBeamLine::LoadInputParameters("<< pedestalfile<<")\n";
//
//    QwParameterFile mapstr(pedestalfile.Data());  //Open the file
//    fDetectorMaps.insert(mapstr.GetParamFileNameContents());
//
//    while (mapstr.ReadNextLine())
//      {
//        lineread+=1;
//        if (ldebug)std::cout<<" line read so far ="<<lineread<<"\n";
//        mapstr.TrimComment('!');   // Remove everything after a '!' character.
//        mapstr.TrimWhitespace();   // Get rid of leading and trailing spaces.
//        if (mapstr.LineIsEmpty())  continue;
//        else {
//            varname = mapstr.GetTypedNextToken<TString>();      //name of the channel
//            varname.ToLower();
//            varname.Remove(TString::kBoth,' ');
//            varxpos      = mapstr.GetTypedNextToken<Double_t>(); // value of the initial x position
//            varypos      = mapstr.GetTypedNextToken<Double_t>(); // value of the initial y position
//            varxref      = mapstr.GetTypedNextToken<Double_t>(); // value of the final y position
//            varyref      = mapstr.GetTypedNextToken<Double_t>(); // value of the final x position
//            varPMT1      = mapstr.GetTypedNextToken<Double_t>(); // value of the rate from scanner 1
//            varPMT2      = mapstr.GetTypedNextToken<Double_t>(); // value of the rate from scanner 2
//
//            if (ldebug)
//              std::cout << "Inputs for channel "    << varname  << ": ped=" << varped << ": cal=" << varcal << "\n"
//                        << ": X Position="          << varxpos  << "\n"
//                        << ": Y Position="          << varypos  << "\n"
//                        << ": X Reference="         << varxref  << "\n"
//                        << ": Y Reference="         << varyref  << "\n"
//                        << ": Rate from Scanner 1=" << varPMT1  << "\n"
//                        << ": Rate from Scanner 2=" << varPMT2  << "\n";
//
//            // Bool_t notfound=kTRUE;
//
//            // if (notfound)
//            for (size_t i=0;i<fIntegrationPMT.size();i++){
//              if (fIntegrationPMT[i].GetElementName()==varname) {
//
//                fIntegrationPMT[i].SetNormRate(varnormrate);
//                fIntegrationPMT[i].SetVoltPerHz(varvoltperhz);
//                fIntegrationPMT[i].SetAsymmetry(varasym);
//                fIntegrationPMT[i].SetCoefficientCx(varcx);
//                fIntegrationPMT[i].SetCoefficientCy(varcy);
//                fIntegrationPMT[i].SetCoefficientCxp(varcxp);
//                fIntegrationPMT[i].SetCoefficientCyp(varcyp);
//                fIntegrationPMT[i].SetCoefficientCe(varce);
//
//                // i=fIntegrationPMT.size()+1;
//                // notfound=kFALSE;
//                // i=fIntegrationPMT.size()+1;
//
//                break;
//
//              }
//            }
//
//        }
//
//    }
//    mapstr.Close(); // Close the file (ifstream)

    return 0;
}

void QwScanner::LoadMockDataParameters(TString pedestalfile) {


    Bool_t ldebug=kTRUE;
    TString varname;
    Double_t varped=0.0;
    Double_t varcal=1.0;

    //  Double_t varbaserate;
    Double_t varscanrate;
    Double_t varscanspeed;
    Double_t varsecaxisstep;
    Double_t varxmin;
    Double_t varxmax;
    Double_t varymin;
    Double_t varymax;
    Double_t varprintfreq;

    TString localname;

    Int_t lineread=0;

    QwParameterFile mapstr(pedestalfile.Data());  //Open the file
    fDetectorMaps.insert(mapstr.GetParamFileNameContents());

    while (mapstr.ReadNextLine()) {

        lineread+=1;
        if (ldebug)std::cout<<" line read so far ="<<lineread<<"\n";
         mapstr.TrimComment('!');   // Remove everything after a '!' character.

        mapstr.TrimWhitespace();   // Get rid of leading and trailing spaces.

        if (mapstr.LineIsEmpty())  continue;

        else {
            varname = mapstr.GetTypedNextToken<TString>();      //name of the channel
            varname.ToLower();
            varname.Remove(TString::kBoth,' ');
            varscanrate    = mapstr.GetTypedNextToken<Double_t>();
            varscanspeed   = mapstr.GetTypedNextToken<Double_t>();
            varsecaxisstep = mapstr.GetTypedNextToken<Double_t>();
            varxmin        = mapstr.GetTypedNextToken<Double_t>();
            varxmax        = mapstr.GetTypedNextToken<Double_t>();
            varymin        = mapstr.GetTypedNextToken<Double_t>();
            varymax        = mapstr.GetTypedNextToken<Double_t>();
            varprintfreq   = mapstr.GetTypedNextToken<Double_t>();

            if (ldebug)
             std::cout << "Inputs for channel "      << varname        << ": ped="  << varped  << ": cal=" << varcal << "\n"
                       << ": Scan Rate="             << varscanrate    << "\n"
                       << ": Scan Speed="            << varscanspeed   << "\n"
                       << ": Secondary Axis Step="   << varsecaxisstep << "\n"
                       << ": Print Frequency="       << varprintfreq   << "\n"
                       << ": Xmin=" << varxmin << ", Xmax=" << varxmax << "\n"
                       << ": Ymin=" << varymin << ", Ymax=" << varymax << "\n";

            // Bool_t notfound=kTRUE;

            // if (notfound)

            fScanRate = varscanrate;
	    fScanSpeed = varscanspeed;
	    fMockSecondaryAxisStep = varsecaxisstep;
	    fxmin = varxmin;
	    fxmax = varxmax;
	    fymin = varymin;
	    fymax = varymax;
        }
    }


    f = TFile::Open("../us_test_XY_r_EG1_65.root");

    h2 = (TH2F*)f->Get("det175/det175_XY_evt_epiM_rate_EG1");
    if (!h2) {
        std::cerr << "Error: histogram not found" << std::endl;
        f->ls();
    }
    h3 = (TH2F*)h2->Clone();

    if (ldebug)
     std::cout<<" line read in the pedestal + cal file ="<<lineread<<" \n";

    ldebug=kFALSE;
    mapstr.Close(); // Close the file (ifstream)


//    Bool_t ldebug=kFALSE;
//    TString varname;
//    Double_t varped;
//    Double_t varcal;
//
//    //  Double_t varbaserate;
//    Double_t varnormrate;
//    Double_t varvoltperhz;
//    Double_t varasym;
//    Double_t varcx;
//    Double_t varcy;
//    Double_t varcxp;
//    Double_t varcyp;
//    Double_t varce;
//
//    TString localname;
//
//    Int_t lineread=0;
//
//    QwParameterFile mapstr(pedestalfile.Data());  //Open the file
//    fDetectorMaps.insert(mapstr.GetParamFileNameContents());
//
//    while (mapstr.ReadNextLine()) {
//
//        lineread+=1;
//        if (ldebug)std::cout<<" line read so far ="<<lineread<<"\n";
//         mapstr.TrimComment('!');   // Remove everything after a '!' character.
//
//        mapstr.TrimWhitespace();   // Get rid of leading and trailing spaces.
//
//        if (mapstr.LineIsEmpty())  continue;
//
//        else {
//            varname = mapstr.GetTypedNextToken<TString>();      //name of the channel
//            varname.ToLower();
//            varname.Remove(TString::kBoth,' ');
//            varxpos      = mapstr.GetTypedNextToken<Double_t>(); // value of the initial x position
//            varypos      = mapstr.GetTypedNextToken<Double_t>(); // value of the initial y position
//            varxref      = mapstr.GetTypedNextToken<Double_t>(); // value of the final y position
//            varyref      = mapstr.GetTypedNextToken<Double_t>(); // value of the final x position
//            varPMT1      = mapstr.GetTypedNextToken<Double_t>(); // value of the rate from scanner 1
//            varPMT2      = mapstr.GetTypedNextToken<Double_t>(); // value of the rate from scanner 2
//
//
//            if (ldebug)
//             std::cout << "Inputs for channel "     << varname  << ": ped="  << varped  << ": cal=" << varcal << "\n"
//                       << ": X Position="           << varxpos   << "\n"
//                       << ": Y Position="           << varypos   << "\n"
//                       << ": X Reference="          << varxref  << "\n"
//                       << ": Y Reference="          << varyref  << "\n"
//                       << ": Rate from Scanner 1="  << varPMT1  << "\n"
//                       << ": Rate from Scanner 2="  << varPMT2  << "\n";
//
//            // Bool_t notfound=kTRUE;
//
//            // if (notfound)
//            for (size_t i=0;i<fIntegrationPMT.size();i++){
//              if (fIntegrationPMT[i].GetElementName()==varname) {
//
//
//		fScanRate[i] = scanrate
//
////                fIntegrationPMT[i].SetNormRate(varnormrate);
////                fIntegrationPMT[i].SetVoltPerHz(varvoltperhz);
////                fIntegrationPMT[i].SetAsymmetry(varasym);
////                fIntegrationPMT[i].SetCoefficientCx(varcx);
////                fIntegrationPMT[i].SetCoefficientCy(varcy);
////                fIntegrationPMT[i].SetCoefficientCxp(varcxp);
////                fIntegrationPMT[i].SetCoefficientCyp(varcyp);
////                fIntegrationPMT[i].SetCoefficientCe(varce);
//
//                // i=fIntegrationPMT.size()+1;
//                // notfound=kFALSE;
//                // i=fIntegrationPMT.size()+1;
//
//                break;
//
//              }
//            }
//
//        }
//
//    }
//
//
//    f = TFile::Open("../us_test_XY_r_EG1_65.root");
//
//    h2 = (TH2F*)f->Get("det175/det175_XY_evt_epiM_rate_EG1");
//    if (!h2) {
//        std::cerr << "Error: histogram not found" << std::endl;
//        f->ls();
//        return;
//    }
//    h3 = (TH2F*)h2->Clone();
//
//    if (ldebug)
//     std::cout<<" line read in the pedestal + cal file ="<<lineread<<" \n";
//
//    ldebug=kFALSE;
//    mapstr.Close(); // Close the file (ifstream)
}



void  QwScanner::RandomizeEventData(int helicity = 0, Double_t time = 1.0)
{

//    Double_t speed = fScanSpeed; // scanner speed
    Double_t step = 0.0;
    Double_t timestep = 0.0;

	if (first_time == kTRUE) {
		x_pos = fxmin+0.001;
		y_pos = fymin+0.001;
		old_time = time;
		first_time = kFALSE;
	} else {
		timestep = time - old_time;
		step = timestep*fScanSpeed;
		old_time = time;
	}
	std::cout << "step=" << step << std::endl;
        std::cout << "time=" << time << std::endl;
        std::cout << "timestep=" << timestep << std::endl;
        std::cout << "vert=" << vert << std::endl;
/*
        std::cout << "QwScanner is working" << std::endl;
        Double_t time_increment = .1; //scan rate
        Double_t start_num_ts = start_time / time_increment;
        Double_t num_ts = time / time_increment;
        Double_t speed = 10000; // scanner speed
        Double_t step = speed*time_increment;
        int num_y_pos = 0;
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
        Double_t x_min = -600;
        Double_t y_min = -1400;
        Double_t x_max = 600;
        Double_t y_max = -800; // in mm
        int x_bins = (x_max-x_min)/step;
        int y_bins = (y_max-y_min)/step;
        Double_t delta_x = 1/time_increment;
        int print_step_size = 10000;
        int fill_step_size = 1;
        Double_t x_pos = x_min;
        Double_t y_pos = y_min;
        Double_t y_goal = y_min;
        Double_t mod_x_pos;
        Double_t mod_y_pos;
        Double_t y_track_sep = 1; // in mm

        TFile *f = TFile::Open("us_test_XY_r_EG1_65.root");

        TH2F *h2 = (TH2F*)f->Get("det175/det175_XY_evt_epiM_rate_EG1");
        if (!h2) {
                std::cerr << "Error: histogram not found" << std::endl;
                f->ls();
                return;
        }
        TH2F *h3 = (TH2F*)h2->Clone();

        // loop to get starting position
        for (int i = 0; i < start_num_ts; i++) {
                if (y_pos > y_max) {
                        y_goal = y_min;
                        y_pos = y_min;
                        num_y_pos = 0;
                        x_pos = x_min;
                        vert = false;
                }

                if (vert == true) {}
                else if (num_y_pos % 2 == 0) x_pos += step;
                else if (num_y_pos % 2 == 1) x_pos -= step;

                if (vert == false && (x_pos >= x_max || x_pos <= x_min)) {
                        vert = true;
                        y_goal = y_pos + y_track_sep;
                }
                else if (vert == true && y_pos < y_goal - step) {
                        y_pos += step;
                }
                else if (vert == true) {
                        vert = false;
                        y_pos += step;
                        num_y_pos += 1;
                }
        }
*/
        // loop for all calculations
        if (y_pos > fymax) {
                y_goal = fymin;
                y_pos = fymin;
                num_y_pos = 0;
                x_pos = fxmin;
                vert = false;
                h3->Reset();
        }
        if (vert == true) {}
        else if (num_y_pos % 2 == 0) x_pos += step;
        else if (num_y_pos % 2 == 1) x_pos -= step;
        if (vert == false && (x_pos >= fxmax || x_pos <= fxmin)) {
                vert = true;
                y_goal = y_pos + y_track_sep; // y_goal and y_pos are in mm
        }
        else if (vert == true && y_pos < y_goal - step) {
                y_pos += step;
        }
        else if (vert == true) {
                vert = false;
                y_pos += step;
                num_y_pos += 1;
        }
//                std::cout << "vert =" << vert << ", x_pos = " << x_pos << ", y_pos = " << y_pos << ", y_goal = " << y_goal << std::endl;

	// end of if first_time = kFALSE

	std::cout << "x pos = " << x_pos << ", y pos = " << y_pos << std::endl;

        xbin = h2->GetXaxis()->FindFixBin(x_pos);
        ybin = h2->GetYaxis()->FindFixBin(y_pos);
        Double_t bin1_x_center = h2->GetXaxis()->GetBinCenter(xbin);
        Double_t bin1_y_center = h2->GetYaxis()->GetBinCenter(ybin);
        Double_t x_width = h2->GetXaxis()->GetBinWidth(xbin);
        Double_t y_width = h2->GetYaxis()->GetBinWidth(ybin);
        mod_x_pos = (x_pos - bin1_x_center)/x_width;
        mod_y_pos = (y_pos - bin1_y_center)/y_width;

        bin1 = h2->GetBinContent(xbin, ybin);
                // if x/y width!=10, print error
                // print error if bin_num <1 or >num_bins in whole histogram, not in scanning range
                        // also find what values are given for either side of histogram
                //else if ==0
        if (mod_x_pos > 0) {
                bin2 = h2->GetBinContent(xbin+1, ybin);
                if (mod_y_pos > 0) {
                        bin3 = h2->GetBinContent(xbin, ybin+1);
                        bin4 = h2->GetBinContent(xbin+1, ybin+1);
                }
                else {
                        bin3 = h2->GetBinContent(xbin, ybin-1);
                        bin4 = h2->GetBinContent(xbin+1, ybin-1);
                }
        }
        else {
                bin2 = h2->GetBinContent(xbin-1, ybin);
                if (mod_y_pos > 0) {
                        bin3 = h2->GetBinContent(xbin, ybin+1);
                        bin4 = h2->GetBinContent(xbin-1, ybin+1);
                }
                else {
                        bin3 = h2->GetBinContent(xbin, ybin-1);
                        bin4 = h2->GetBinContent(xbin-1, ybin-1);
                }
        }

        mod_x_pos = std::abs(mod_x_pos);
        mod_y_pos = std::abs(mod_y_pos);

        x_lin_val = ((1-mod_x_pos) * bin1 + mod_x_pos * bin2)/2+((1-mod_x_pos) * bin3 + mod_x_pos * bin4)/2;
        y_lin_val = ((1-mod_y_pos) * bin1 + mod_y_pos * bin3)/2+((1-mod_y_pos) * bin2 + mod_y_pos * bin4)/2;
        lin_val = (1-mod_x_pos) * (1-mod_y_pos) * bin1 + (mod_x_pos) * (1-mod_y_pos) * bin2 + (1-mod_x_pos) * (mod_y_pos) * bin3 + (mod_x_pos) * (mod_y_pos) * bin4;
//	std::cout << "fill_step_size = " << fill_step_size << std::endl;
//        std::cout << "print_step_size = " << print_step_size << std::endl;
//        if (timestep % fill_step_size == 0) h3->Fill(x_pos, y_pos,100);
        if (//timestep % print_step_size == 0 && 
//		(bin1 > 1 || bin2 > 1 || bin3 > 1 || bin4 > 1)
		1==1) {
                std::cout << "i: " << timestep << std::endl;
                std::cout << "  bin1 value: " << bin1 << ",     bin2 value: " << bin2 << std::endl;
                std::cout << "  bin3 value: " << bin3 << ",     bin4 value: " << bin4 << std::endl;
                std::cout << "  bin 1&3 x dist: " << mod_x_pos << "     bin 2&4 x dist: " << 1-mod_x_pos << std::endl;
                std::cout << "  bin 1&2 y dist: " << mod_y_pos << "     bin 3&4 y dist: " << 1-mod_y_pos << std::endl;
                std::cout << "  bin 1 weight: " <<  (1-mod_x_pos) * (1-mod_y_pos) << "  bin 2 weight: " << (mod_x_pos) * (1-mod_y_pos) << std::endl;
                std::cout << "  bin 3 weight: " <<  (1-mod_x_pos) * (mod_y_pos) << "    bin 4 weight: " << (mod_x_pos) * (mod_y_pos) << std::endl;
                std::cout << "  total linear value: " << lin_val << std::endl;
                std::cout << "  x linear value: " << x_lin_val << ",    y linear value: " << y_lin_val << std::endl;
                std::cout << "  xbin (column) #: " << xbin << ",        ybin (row) #: " << ybin << std::endl;
                std::cout << "  x bin center: " << bin1_x_center << ",  y bin center: " << bin1_y_center << std::endl;
                std::cout << "  x bin width: " << x_width << ", y bin width: " << y_width << std::endl;
                std::cout << "  x_pos: " << x_pos << ", y_pos: " << y_pos << std::endl;
        }

	h2->Draw("COLZ");
        h3->Draw("COLZSAME");
        std::cout << "Final x position = " << x_pos << "\n";
        std::cout << "Final y position = " << y_pos << "\n";
	// add time printout
//	timestep += 1;

	fPosition.first = x_pos;
	fPosition.second = y_pos;
	fPMTrate = lin_val;
}


void QwScanner::UpdateErrorFlag(VQwSubsystem const*) {}
void QwScanner::EncodeEventData(std::vector<UInt_t> &buffer) {

    std::cout << "EncodeEventData Started" << std::endl;

    bIsExchangedDataValid = kTRUE;

    if (1==1 || bNormalization) {

        if(RequestExternalValue("q_targ", &fTargetCharge)) {

            if (bDEBUG) {

                    QwWarning << "QwScanner::ExchangeProcessedData Found "<<fTargetCharge.GetElementName()<< QwLog::endl;
                    (dynamic_cast<QwMollerADC_Channel*>(&fTargetCharge))->PrintInfo();

            }

        } else {

            bIsExchangedDataValid = kFALSE;
            QwError << GetName() << " could not get external value for "
                 << fTargetCharge.GetElementName() << QwLog::endl;

        }
    }


    // calculate position voltages
    (fXY.first) = (fPositionReference.first);
    (fXY.first).Scale((fPosition.first - fPositionOrigin.first)/(fPositionFullScale.first));
    (fXY.second) = (fPositionReference.second);
    (fXY.second).Scale((fPosition.second - fPositionOrigin.second)/(fPositionFullScale.second));

    Double_t SignalVoltage = fPMTrate*fTargetCharge.GetValue()*fVoltPerHz;

    std::vector<UInt_t> elements;
    elements.clear();

    // Get all buffers in the order they are defined in the map file
//    for (size_t i = 0; i < fMainDetID.size(); i++) {
//
//        // This is a QwIntegrationPMT
//        if (fMainDetID.at(i).fTypeID == kQwIntegrationPMT)
//         fScanner[fMainDetID.at(i).fIndex]->EncodeEventData(elements);
//
//    }


    //QwWarning << "QwScanner::ExchangeProcessedData "<< QwLog::endl;
}

Int_t QwScanner::ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) {

    Bool_t lkDEBUG=kFALSE;

    Int_t index = GetSubbankIndex(roc_id,bank_id);

    if (index>=0 && num_words>0) {

        //  We want to process this ROC.  Begin looping through the data.
        if (lkDEBUG)
            std::cout << "QwScanner::ProcessEvBuffer:  "
             << "Begin processing ROC" << roc_id
             << " and subbank "<<bank_id
             << " number of words="<<num_words<<std::endl;

        for (size_t i=0;i<fMainDetID.size();i++) {

            if (fMainDetID[i].fSubbankIndex==index) {

                if (fMainDetID[i].fTypeID == kQwIntegrationPMT) {

                    if (lkDEBUG) {

                        std::cout<<"found IntegrationPMT data for "<<fMainDetID[i].fdetectorname<<std::endl;
                        std::cout<<"word left to read in this buffer:"<<num_words-fMainDetID[i].fWordInSubbank<<std::endl;

                    }

                    fIntegrationPMT[fMainDetID[i].fIndex].ProcessEvBuffer(&(buffer[fMainDetID[i].fWordInSubbank]),
                     num_words-fMainDetID[i].fWordInSubbank);

                }
            }
        }
    }

  return 0;
}
EQwPMTInstrumentType QwScanner::GetDetectorTypeID(TString name) {

    return GetQwPMTInstrumentType(name);

}

Int_t QwScanner::GetDetectorIndex(EQwPMTInstrumentType type_id, TString name) {
    Bool_t ldebug=kFALSE;

    if (ldebug) {

        std::cout<<"QwScanner::GetScannerIndex\n";
        std::cout<<"type_id=="<<type_id<<" name="<<name<<"\n";
        std::cout<<fMainDetID.size()<<" already registered detector\n";
    }

    Int_t result=-1;
    for (size_t i=0;i<fMainDetID.size();i++) {

        if (fMainDetID[i].fTypeID==type_id)
         if (fMainDetID[i].fdetectorname==name) {

            result=fMainDetID[i].fIndex;

            if (ldebug)
             std::cout<<"testing against ("<<fMainDetID[i].fTypeID
             <<","<<fMainDetID[i].fdetectorname<<")=>"<<result<<"\n";

         }
    }

    return result;

}
void QwScanner::IncrementErrorCounters() {}
void QwScanner::Ratio(VQwSubsystem*, VQwSubsystem*) {}
Bool_t QwScanner::ApplySingleEventCuts()
{
	return true;
}
//void virtual thunk to QwScanner::operator=(VQwSubsystem*) {}

// Operator commands
VQwSubsystem& QwScanner::operator=(VQwSubsystem *value)
{
	if (Compare(value)) {
	        QwScanner* input = dynamic_cast<QwScanner*> (value);

	        for (size_t i=0;i<input->fScanner.size();i++)
	         *this->fScanner[i]=*input->fScanner[i];
	}
        return *this;
}
VQwSubsystem& QwScanner::operator+=(VQwSubsystem *value)
{
    if (Compare(value)) {
        QwScanner* input = dynamic_cast<QwScanner*> (value);

        for (size_t i=0;i<input->fScanner.size();i++)
         *this->fScanner[i]+=*input->fScanner[i];
    }

    return *this;
}
VQwSubsystem& QwScanner::operator-=(VQwSubsystem *value)
{
    if (Compare(value)) {
        QwScanner* input = dynamic_cast<QwScanner*> (value);

        for (size_t i=0;i<input->fScanner.size();i++)
         *this->fScanner[i]-=*input->fScanner[i];
    }

    return *this;
}
void QwScanner::PrintErrorCounters() const {}
void QwScanner::DeaccumulateRunningSum(VQwSubsystem*, int) {}
//void virtual thunk to QwScanner::EncodeEventData(std::vector<unsigned int, std::allocator<unsigned int> >&) {}
UInt_t QwScanner::GetEventcutErrorFlag()
{
	return 0;
}
void QwScanner::Scale(double) {}
void QwScanner::CalculateRunningAverage() {}
void QwScanner::AccumulateRunningSum(VQwSubsystem*, int, int) {}



/**
 * Clear the event data in this subsystem
 */
void QwScanner::ClearEventData()
{
  // Clear all Scanner channels
  for (size_t i = 0; i < fScanner.size(); i++) {
    fScanner.at(i)->ClearEventData();
  }
  // Reset good event count
  fGoodEventCount = 0;
}


void QwScanner::ProcessEvent()
{
  // Process the event
  fTestValue += 1.0;
  for (size_t i = 0; i < fScanner.size(); i++) {
    fScanner.at(i)->ProcessEvent();
  }
}


void QwScanner::ConstructHistograms(TDirectory* folder, TString& prefix)
{
  for(size_t i = 0; i < fScanner.size(); i++) {
    fScanner.at(i)->ConstructHistograms(folder, prefix);
  }
}

void QwScanner::FillHistograms()
{
  for(size_t i = 0; i < fScanner.size(); i++) {
    fScanner.at(i)->FillHistograms();
  }
}

void QwScanner::ConstructBranchAndVector(TTree *tree, TString & prefix, std::vector <Double_t> &values) {

  for (size_t i = 0; i < fScanner.size(); i++) {
    fScanner.at(i)->ConstructBranchAndVector(tree, prefix, values);
  }
}

void QwScanner::FillTreeVector(std::vector<Double_t> &values) const
{
  for(size_t i = 0; i < fScanner.size(); i++) {
    fScanner.at(i)->FillTreeVector(values);
  }
}

/**
 * Compare two Scanner objects
 * @param value Object to compare with
 * @return kTRUE if the object is equal
 */
Bool_t QwScanner::Compare(VQwSubsystem *value)
{
  // Immediately fail on null objects
  if (value == 0) return kFALSE;

  // Continue testing on actual object
  Bool_t result = kTRUE;
  if (typeid(*value) != typeid(*this)) {
    result = kFALSE;

  } else {
    QwScanner* input = dynamic_cast<QwScanner*> (value);
    if (input->fScanner.size() != fScanner.size()) {
      result = kFALSE;
    }
  }
  return result;
}

/**
 * Print some debugging output for the subcomponents
 */
void QwScanner::PrintInfo() const
{
//  VQwSubsystemParity::PrintInfo();
//
//  QwOut << " there are " << fScanner.size() << " scanner channels" << QwLog::endl;
//
//  for (size_t i = 0; i < fScanner.size(); i++) {
//    QwOut << " scanner " << i << ": ";
//    fScanner.at(i)->PrintInfo();
//  }
}

/**
 * Print the value for the subcomponents
 */
void QwScanner::PrintValue() const
{
  QwMessage << "=== QwScanner: " << GetName() << " ===" << QwLog::endl;
  for(size_t i = 0; i < fScanner.size(); i++) {
    fScanner.at(i)->PrintValue();
  }
}

