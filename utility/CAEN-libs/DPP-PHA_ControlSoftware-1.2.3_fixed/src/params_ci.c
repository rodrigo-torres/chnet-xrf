#include "dppswitch.h"

#ifdef DPP_CI


#include "Configurator.h"
#include "messages.h"

#include <stdio.h>

UPDATE(uOpen) {
	Digitizer* dig = UPDATE_GET_DIG();
	char linkTypeString[15];
	unsigned int param1=0, param2=0, param3=0;
	if (sscanf(UPDATE_VALUE,"%15s", linkTypeString)==1) {
		if (strcmp(linkTypeString,"USB")==0) { 
			dig->linkType = CAEN_DGTZ_USB;
			if (sscanf(UPDATE_VALUE,"%15s %d %x %x",linkTypeString,&param1,&param2,&param3)==4) {
				dig->linkNum = param1;
				dig->baseAddress = param3;
			} else {
				dig->linkNum = param1;
				dig->baseAddress = param2;
			}
		} else if (strcmp(linkTypeString,"PCI")==0) {
			dig->linkType = CAEN_DGTZ_PCI_OpticalLink;
			sscanf(UPDATE_VALUE,"%15s %d %x %x",linkTypeString,&param1,&param2,&param3);
			dig->linkNum = param1;
			dig->conetNode = param2;
			dig->baseAddress = param3;
		} else
			userMsg("Config OPEN: Invalid connection type \"%s\"\n",linkTypeString); 
	} else 
		userMsg("Config OPEN: Invalid value string \"%s\"\n",UPDATE_VALUE);
}

UPDATE(uRecordLength) { 
	Digitizer* dig = UPDATE_GET_DIG();
	dig->recordLength = atoi(UPDATE_VALUE);
	UPDATE_RESTART();
}

UPDATE(uNumAggregates) {
	UPDATE_GET_DIG()->numAggregates = atoi(UPDATE_VALUE);
}

UPDATE(uNumEventPerAggr) {
    Digitizer *dig = UPDATE_GET_DIG();
    dig->nEvtAggr = atoi(UPDATE_VALUE);
    if(dig->nEvtAggr<0)
        dig->nEvtAggr = 0;
    if(dig->nEvtAggr>1023)
        dig->nEvtAggr = 1023;
    UPDATE_RESTART();
}

UPDATE(uPulsePolarity) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i;
		for (i=0; i<MAX_CH; i++)
			if (_stricmp(UPDATE_VALUE,"POSITIVE")==0) dig->pulsePolarity[i] = 0;
			else if (_stricmp(UPDATE_VALUE,"NEGATIVE")==0) dig->pulsePolarity[i] = 1;
	} else {
			if (_stricmp(UPDATE_VALUE,"POSITIVE")==0) dig->pulsePolarity[atoi(UPDATE_SECTION)] = 0;
			else if (_stricmp(UPDATE_VALUE,"NEGATIVE")==0) dig->pulsePolarity[atoi(UPDATE_SECTION)] = 1;
	}
	UPDATE_RESTART();
}

UPDATE(uFreeWrite) {
	Digitizer* dig = UPDATE_GET_DIG();
	uint32_t address=0, value=0;
	if (!freeWritesFull(dig) && sscanf(UPDATE_VALUE, "%x %x", &address, &value)==2) {
		addFreeWrite(dig,address,value);
	}
}

UPDATE(uExternalTrigger) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_VALUE,"DISABLED")==0) dig->extTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED; 
	else if (_stricmp(UPDATE_VALUE,"ACQUISITION_ONLY")==0) dig->extTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
	else if (_stricmp(UPDATE_VALUE,"ACQUISITION_AND_TRGOUT")==0) dig->extTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
	else userMsg("Config %s: invalid value string \"%s\"\n",UPDATE_KEY, UPDATE_VALUE);
}

UPDATE(uGatedStart) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_VALUE,"DISABLED")==0) dig->gatedStartEnabled = 0; 
	else if (_stricmp(UPDATE_VALUE,"ENABLED")==0) dig->gatedStartEnabled = 1;
	else userMsg("Config %s: invalid value string \"%s\"\n",UPDATE_KEY, UPDATE_VALUE);
}

UPDATE(uAcqMode) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_VALUE,"OSCILLOSCOPE")==0) { 
		dig->acquisitionMode = CAEN_DGTZ_DPP_ACQ_MODE_Mixed; UPDATE_GET_ANA()->plotType = PLOT_WAVEFORMS; 
	} // careful: OSCILLOSCOPE is actually MIXED mode!
	else if (_stricmp(UPDATE_VALUE,"LIST")==0) { 
		dig->signalTimeReset = 1;
		dig->acquisitionMode = CAEN_DGTZ_DPP_ACQ_MODE_List; UPDATE_GET_ANA()->plotType = PLOT_EHISTO; 
	}
	else if (_stricmp(UPDATE_VALUE,"MIXED")==0) dig->acquisitionMode = CAEN_DGTZ_DPP_ACQ_MODE_Mixed;
	else userMsg("Config %s: invalid value string \"%s\"\n",UPDATE_KEY, UPDATE_VALUE);
	UPDATE_RESTART();
}

UPDATE(uListMode) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_VALUE,"ENERGY_ONLY")==0) dig->listParams = CAEN_DGTZ_DPP_SAVE_PARAM_EnergyOnly;
	else if (_stricmp(UPDATE_VALUE, "TIME_ONLY")==0) dig->listParams = CAEN_DGTZ_DPP_SAVE_PARAM_TimeOnly;
	else if (strcmp(UPDATE_VALUE, "ENERGY_AND_TIME")==0) dig->listParams = CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime;
    else userMsg("Config %s: Invalid value string \"%s\"\n", UPDATE_KEY, UPDATE_VALUE);
	UPDATE_RESTART();
}

UPDATE(uMaxEvents) {
	UPDATE_GET_DIG()->maxNumEvents = atoi(UPDATE_VALUE);
}

UPDATE(uPreTrigger) {
	Digitizer* dig = UPDATE_GET_DIG();
	dig->preTrigger = atoi(UPDATE_VALUE)/dig->tsampl[0]; // HACK here we use the tsampl from the first ch
	UPDATE_RESTART();
}

UPDATE(uTrgHoldoff) {
    Digitizer* dig = UPDATE_GET_DIG();
    dig->dppParams.trgho = atoi(UPDATE_VALUE);
    UPDATE_RESTART();
}

UPDATE(uDppTrgThreshold) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) dig->dppParams.thr[i] = atoi(UPDATE_VALUE);
	} else {
		dig->dppParams.thr[atoi(UPDATE_SECTION)] = atoi(UPDATE_VALUE);
	}
	UPDATE_RESTART();
}


UPDATE(uTrgConfig) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) dig->dppParams.trgc[i] = strcmp(UPDATE_VALUE,"PEAK")==0 ? 0 : 1;
	} else {
		dig->dppParams.trgc[atoi(UPDATE_SECTION)] = strcmp(UPDATE_VALUE,"PEAK")==0 ? 0 : 1;
	}	
	UPDATE_RESTART();
}

UPDATE(uTrgMode) {
	Digitizer* dig = UPDATE_GET_DIG();
	dig->trgMode = strcmp(UPDATE_VALUE,"NORMAL")==0 ? 0 : 1;
	UPDATE_RESTART();
}

UPDATE(uSelfTrigger) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) dig->dppParams.selft[i] = strcmp(UPDATE_VALUE,"ENABLED")==0 ? 1 : 0;
	} else {
		dig->dppParams.selft[atoi(UPDATE_SECTION)] = strcmp(UPDATE_VALUE,"ENABLED")==0 ? 1 : 0;
	}	
	UPDATE_RESTART();
}


UPDATE(uGate) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) dig->dppParams.gate[i] = atoi(UPDATE_VALUE)/dig->tsampl[i];
	} else {
		dig->dppParams.gate[atoi(UPDATE_SECTION)] = atoi(UPDATE_VALUE)/dig->tsampl[atoi(UPDATE_SECTION)];
	}
	UPDATE_RESTART();
}


UPDATE(uPreGate) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i;
		for (i=0; i<MAX_CH; i++)
			dig->dppParams.pgate[i] = atoi(UPDATE_VALUE)/dig->tsampl[i];
	} else {
		dig->dppParams.pgate[atoi(UPDATE_SECTION)] = atoi(UPDATE_VALUE)/dig->tsampl[atoi(UPDATE_SECTION)];
	}
	UPDATE_RESTART();
}


/*
UPDATE(uBaselineThreshold) {
	Digitizer* dig = UPDATE_GET_DIG();
	dig->dppBaselineThresh = atoi(UPDATE_VALUE);
	UPDATE_GET_ANA()->underflowShown = 0;
	UPDATE_RESTART();
}
*/
UPDATE(uBaselineThreshold) {
	Digitizer* dig = UPDATE_GET_DIG();
	dig->dppParams.blthr = atoi(UPDATE_VALUE);
	UPDATE_GET_ANA()->underflowShown = 0;
	UPDATE_RESTART();
}


/*
UPDATE(uBaselineTimeout) {
	Digitizer* dig = UPDATE_GET_DIG();
	dig->dppBaselineTimeout = atoi(UPDATE_VALUE)/32;
	UPDATE_RESTART();
}
*/
UPDATE(uBaselineTimeout) {
	Digitizer* dig = UPDATE_GET_DIG();
	dig->dppParams.bltmo = atoi(UPDATE_VALUE);
	UPDATE_GET_ANA()->underflowShown = 0;
	UPDATE_RESTART();
}


UPDATE(uTimeValAcqWin) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i;
		for (i=0; i<MAX_CH; i++)
			dig->dppParams.tvaw[i] = atoi(UPDATE_VALUE)/dig->tsampl[i];
	} else {
		dig->dppParams.tvaw[atoi(UPDATE_SECTION)] = atoi(UPDATE_VALUE)/dig->tsampl[atoi(UPDATE_SECTION)];
	}
	UPDATE_RESTART();
}


/*
UPDATE(uPURGap) {
	Digitizer* dig = UPDATE_GET_DIG();
	dig->dppPurGap = atoi(UPDATE_VALUE)/dig->tsampl[0];
	UPDATE_RESTART();
}
*/

//UPDATE(uPURGap) { // HACK! PUR was removed in DPP-CI for x720
//	Digitizer* dig = UPDATE_GET_DIG();
//    dig->dppParams.purgap = atoi(UPDATE_VALUE)/dig->tsampl[0];
//	UPDATE_RESTART();
//}

/*
UPDATE(uPURMode) {
	Digitizer* dig = UPDATE_GET_DIG();
	dig->dppPurRej = _stricmp(UPDATE_VALUE,"REJECT")==0;
	UPDATE_RESTART();
}
*/

//UPDATE(uPURMode) { // HACK! PUR was removed in DPP-CI for x720
//	Digitizer* dig = UPDATE_GET_DIG();
//    dig->dppParams.purh = (_stricmp(UPDATE_VALUE,"REJECT")==0)? CAEN_DGTZ_DPP_PSD_PUR_Enabled : CAEN_DGTZ_DPP_PSD_PUR_DetectOnly;
//	UPDATE_RESTART();
//}


UPDATE(uDppOptBaseline) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++)
            dig->dppParams.nsbl[i] = atoi(UPDATE_VALUE);
	} else {
		dig->dppParams.nsbl[atoi(UPDATE_SECTION)] = atoi(UPDATE_VALUE);
	}
	UPDATE_RESTART();
}


UPDATE(uDcOffset) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) {
            if((dig->enableMask>>i)&0x1) {
			    dig->dcOffset[i]  = (int)((atof(UPDATE_VALUE)+50) * 65535 / 100);
			    if (checkBoardInited(dig)) CAEN_DGTZ_SetChannelDCOffset(dig->boardHandle, i, dig->dcOffset[i]);
            }
		}
	} else {
        int i = atoi(UPDATE_SECTION);
        if((dig->enableMask>>i)&0x1) {
		    dig->dcOffset[i] = (int)((atof(UPDATE_VALUE)+50) * 65535 / 100);
		    if (checkBoardInited(dig)) CAEN_DGTZ_SetChannelDCOffset(dig->boardHandle, i, dig->dcOffset[i]);
        }
	}
}

UPDATE(uFPIOLevel) {
	Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_VALUE,"TTL")==0) dig->fpioType = 1;
	else if (_stricmp(UPDATE_VALUE, "NIM")==0) dig->fpioType = 0;
    else userMsg("Config %s: Invalid value string \"%s\"\n", UPDATE_KEY, UPDATE_VALUE);
}

UPDATE(uChannelEnable) {
	Digitizer* dig = UPDATE_GET_DIG();
    uint32_t physChMask = (0x1<<dig->numChannels)-1;
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		//int i=0;
		dig->enableMask = (_stricmp(UPDATE_VALUE,"YES")==0) ? 0xFF : 0;
	} else {
		if (_stricmp(UPDATE_VALUE,"YES")==0) dig->enableMask |= (1 << atoi(UPDATE_SECTION));
		else dig->enableMask &= ~(1 << atoi(UPDATE_SECTION));
	}
    dig->enableMask &= physChMask;
	UPDATE_RESTART(); /* fiddling with channel enable requires a runner restart b/c memory has to be allocated when a channel is newly enabled */
}



UPDATE(uVirtualProbeA) {
	UPDATE_GET_ANA()->vprobes[1] = atoi(UPDATE_VALUE);
	updateVProbes(UPDATE_GET_ANA());
}

UPDATE(uVirtualProbeD1) {
	UPDATE_GET_ANA()->vprobes[4] = atoi(UPDATE_VALUE);
	updateVProbes(UPDATE_GET_ANA());
}

UPDATE(uVirtualProbeD2) {
	UPDATE_GET_ANA()->vprobes[5] = atoi(UPDATE_VALUE);
	updateVProbes(UPDATE_GET_ANA());
}

UPDATE(uDualTrace) {
	if (_stricmp(UPDATE_VALUE,"YES")==0) UPDATE_GET_ANA()->dualTrace = 1;
	else if (_stricmp(UPDATE_VALUE,"NO")==0) UPDATE_GET_ANA()->dualTrace = 0;
	else userMsg("Config %s: Invalid value string\n", UPDATE_KEY);
	UPDATE_RESTART();
}

UPDATE(uTimeHistoMax) {
	UPDATE_GET_ANA()->tmaxHisto = (int)(atof(UPDATE_VALUE)*1000);
}

UPDATE(uDTCTimeInterval) {
	UPDATE_GET_ANA()->dtcTime = (int)(atof(UPDATE_VALUE)*1000);
}

UPDATE(uDTCEventInterval) {
	UPDATE_GET_ANA()->dtcEvents = atoi(UPDATE_VALUE); 
}

UPDATE(uEminSkim) {
	UPDATE_GET_ANA()->eminSkim = (float)atof(UPDATE_VALUE);
}

UPDATE(uEmaxSkim) {
	UPDATE_GET_ANA()->emaxSkim = (float)atof(UPDATE_VALUE);
}

UPDATE(uEnableEnergySkim) {
	UPDATE_GET_ANA()->eSkimEnabled = _stricmp(UPDATE_VALUE,"YES")==0;
}

UPDATE(uStopTime) {
	Analyzer* ana = UPDATE_GET_ANA();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) ana->stopTime[i]  = (uint64_t)atof(UPDATE_VALUE)*1000000000;
	} else {
		ana->stopTime[atoi(UPDATE_SECTION)] = (uint64_t)atof(UPDATE_VALUE)*1000000000;
	}
}


UPDATE(uGnuplotPath) {
	UPDATE_GET_PLO()->plotterPath = calloc(strlen(UPDATE_VALUE)+1,sizeof(char));
	strcpy(UPDATE_GET_PLO()->plotterPath,UPDATE_VALUE);
}

UPDATE(uVirtualProbeA1Gain) {
	UPDATE_GET_ANA()->traceGains[0] = atoi(UPDATE_VALUE);
}

UPDATE(uVirtualProbeA2Gain) {
	UPDATE_GET_ANA()->traceGains[1] = atoi(UPDATE_VALUE);
}

UPDATE(uVirtualProbeDGain) {
	UPDATE_GET_ANA()->traceGains[2] = atoi(UPDATE_VALUE);
}

UPDATE(uVirtualProbeD2Gain) {
	UPDATE_GET_ANA()->traceGains[3] = atoi(UPDATE_VALUE);
}

UPDATE(uVirtualProbeA1Off) {
	UPDATE_GET_ANA()->traceOffsets[0] = atoi(UPDATE_VALUE);
}

UPDATE(uVirtualProbeA2Off) {
	UPDATE_GET_ANA()->traceOffsets[1] = atoi(UPDATE_VALUE);
}

UPDATE(uVirtualProbeDOff) {
	UPDATE_GET_ANA()->traceOffsets[2] = atoi(UPDATE_VALUE);
}

UPDATE(uVirtualProbeD2Off) {
	UPDATE_GET_ANA()->traceOffsets[3] = atoi(UPDATE_VALUE);
}

UPDATE(uVirtualProbeA1En) {
	UPDATE_GET_ANA()->traceEnableWaves[0] = !_stricmp(UPDATE_VALUE,"YES");
}

UPDATE(uVirtualProbeA2En) {
	UPDATE_GET_ANA()->traceEnableWaves[1] = !_stricmp(UPDATE_VALUE,"YES");
}

UPDATE(uVirtualProbeDEn) {
	UPDATE_GET_ANA()->traceEnableWaves[2] = !_stricmp(UPDATE_VALUE,"YES");
}

UPDATE(uVirtualProbeD2En) {
	UPDATE_GET_ANA()->traceEnableWaves[3] = !_stricmp(UPDATE_VALUE,"YES");
}

UPDATE(uCalibrationX1) {
	Analyzer* ana = UPDATE_GET_ANA();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) { 
			ana->calib[i].x0 = atoi(UPDATE_VALUE);
		}
	} else {
		ana->calib[atoi(UPDATE_SECTION)].x0 = atoi(UPDATE_VALUE);
	}
}

UPDATE(uCalibrationX2) {
	Analyzer* ana = UPDATE_GET_ANA();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) { 
			ana->calib[i].x1 = atoi(UPDATE_VALUE);
		}
	} else {
		ana->calib[atoi(UPDATE_SECTION)].x1 = atoi(UPDATE_VALUE);
	}
}

UPDATE(uCalibrationY1) {
	Analyzer* ana = UPDATE_GET_ANA();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) { 
			ana->calib[i].y0 = (float)atof(UPDATE_VALUE);
		}
	} else {
		ana->calib[atoi(UPDATE_SECTION)].y0 = (float)atof(UPDATE_VALUE);
	}
}

UPDATE(uCalibrationY2) {
	Analyzer* ana = UPDATE_GET_ANA();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) { 
			ana->calib[i].y1 = (float)atof(UPDATE_VALUE);
		}
	} else {
		ana->calib[atoi(UPDATE_SECTION)].y1 = (float)atof(UPDATE_VALUE);
	}
}

UPDATE(uDumpFileName) {
	Dumper* dpr = UPDATE_GET_DPR();
	dpr->dumpFilePrefix = calloc(strlen(UPDATE_VALUE)+1,sizeof(char));
	strcpy(dpr->dumpFilePrefix,UPDATE_VALUE);
	if (dpr->inited) {
		deinitDumper(dpr);
		initDumper(dpr);
	}
}

UPDATE(uDumpWaveforms) {
	UPDATE_GET_ANA()->dumpWf = !_stricmp(UPDATE_VALUE,"YES");
}

UPDATE(uDumpList) {
    int dump,i;
    Analyzer *av;

    dump = !_stricmp(UPDATE_VALUE,"YES");
    av=UPDATE_GET_ANA();
	av->dumpLs = dump;
    if(dump) {
        int dprErr=0;
        // Open the dumpers for enabled channels
        for(i=0; i<MAX_CH; i++) {
            if (checkChannelEnabled(av->dig, i)) {
                c_DPPDescriptor_t dppdescr;
                c_saveFormat_t dprFmt = av->listOutMode;

                // Init & Open the list dumper
                dppdescr.DPPType = c_DPPType_CI;
                dppdescr.TSType = TYPE_UINT32;
                dppdescr.ENType = TYPE_INT16;
                av->dumper[i] = c_openDumper( av->dpr->fileNames[DUMP_LIST][i], c_mode_W, dprFmt, dppdescr );
                if(av->dumper[i]==NULL)
                    dprErr=1;
            }
        }
        if(dprErr==1) {
            char msg[]="Error opening list dump file. Check write permissions.";
            guiMsg(MSG_ERROR,msg);
            guiMsg(MSG_DMP_OPEN);
            goto CloseDumpers; // Since dumpers are different for different channels, we want to close the one evetually opened
        }
        else
            av->dumpLs = 1;
    }
    else {
CloseDumpers:
        // Close the dumpers
        for(i=0; i<MAX_CH; i++) {
            if(av->dumper[i]!=NULL) {
                //c_closeDumper(&av->dumper[i]);
                c_closeDumper(av->dumper[i]);
                av->dumper[i]=NULL;
            }
        }
        av->dumpLs = 0;
    }
}

UPDATE(uListOutMode) {
    if( !strcmp(UPDATE_VALUE, "ASCII") ) {
        userMsg("Using format ASCII\n");
        UPDATE_GET_ANA()->listOutMode = c_format_ASCII;
    }
    else if( !strcmp(UPDATE_VALUE, "BINARY") ) {
        userMsg("Using format BINARY\n");
        UPDATE_GET_ANA()->listOutMode = c_format_Binary;
    }
    else {
        // ERROR, invalid dump mode, using default (ASCII)
        userMsg("Invalid dump format, using default format ASCII\n");
        UPDATE_GET_ANA()->listOutMode = c_format_Binary;
    }
}

UPDATE(uDumpEHisto) {
	UPDATE_GET_ANA()->dumpEh = !_stricmp(UPDATE_VALUE,"YES");
}

UPDATE(uDumpTHisto) {
	UPDATE_GET_ANA()->dumpTh = !_stricmp(UPDATE_VALUE,"YES");
}

UPDATE(uPlotFps) {
	UPDATE_GET_PLO()->fps = atof(UPDATE_VALUE);
}

UPDATE(uChargeSens) {
    Digitizer* dig = UPDATE_GET_DIG();
	if (_stricmp(UPDATE_SECTION,"COMMON")==0) {
		int i=0;
		for (i=0; i<MAX_CH; i++) dig->dppParams.csens[i] = atoi(UPDATE_VALUE);
	} else {
		dig->dppParams.csens[atoi(UPDATE_SECTION)] = atoi(UPDATE_VALUE);
	}
	UPDATE_GET_ANA()->overflowShown = 0;
	UPDATE_RESTART();
}

/* Configuration parameters definition */

CONFIGURATION_PARAMETERS (

	COMMON_PARAMETER(	uOpen,				"OPEN",					"USB 0 0"			);
	COMMON_PARAMETER(	uNumAggregates,		"MAX_NUM_AGGREGATES_BLT","10"				);
	CHANNEL_PARAMETER(	uPulsePolarity,		"PULSE_POLARITY",		"POSITIVE"			);
	COMMON_PARAMETER(	uFreeWrite,			"WRITE_REGISTER",		NO_DEFAULT_VALUE	);
	COMMON_PARAMETER(	uExternalTrigger,	"EXTERNAL_TRIGGER",		"DISABLED"			);
	COMMON_PARAMETER(	uAcqMode,			"ACQUISITION_MODE",		"OSCILLOSCOPE"		);
	COMMON_PARAMETER(	uNumEventPerAggr,   "NEVT_AGGR",            "0"				    );
	COMMON_PARAMETER(	uListMode,			"LIST_PARAMS",			"ENERGY_AND_TIME"	);
	COMMON_PARAMETER(	uPreTrigger,		"PRE_TRIGGER",			"80"				);
    COMMON_PARAMETER(	uTrgHoldoff,		"TRG_HOLDOFF",			"0"				    );
    COMMON_PARAMETER(	uGatedStart,		"GATED_START",			"DISABLED"			);

	CHANNEL_PARAMETER(	uChannelEnable,		"ENABLE_INPUT",			"YES"				);
	
	CHANNEL_PARAMETER(	uDcOffset,			"DC_OFFSET",			"0"					);
	COMMON_PARAMETER(	uFPIOLevel,			"FPIO_LEVEL",			"NIM"				);

	COMMON_PARAMETER(	uRecordLength,		"RECORD_LENGTH",		"1024"				);

	CHANNEL_PARAMETER(	uDppTrgThreshold,	"TRG_THRESHOLD",		"50"				);
	CHANNEL_PARAMETER(	uTrgConfig,			"TRIGGER_CONFIG",		"THRESHOLD"			);
	COMMON_PARAMETER(	uTrgMode,			"TRIGGER_MODE",			"NORMAL"			);
	CHANNEL_PARAMETER(	uSelfTrigger,		"SELF_TRIGGER",			"ENABLED"			);
	CHANNEL_PARAMETER(  uGate,				"CI_GATE",				"80"				);
	CHANNEL_PARAMETER(  uPreGate,			"CI_PRE_GATE",			"40"				);
	COMMON_PARAMETER(	uBaselineThreshold, "CI_BL_THRESHOLD",		"12"				);
	COMMON_PARAMETER(	uBaselineTimeout,	"CI_BL_TIMEOUT",		"40"				);
	CHANNEL_PARAMETER(	uTimeValAcqWin,		"TIME_VAL_ACQ_WIN",		"80"				);
	//COMMON_PARAMETER(	uPURGap,			"PUR_GAP",				"80"				); // HACK! PUR was removed in DPP-CI for x720
	//COMMON_PARAMETER(   uPURMode,			"PUR_MODE",				"DETECT"			); // HACK! PUR was removed in DPP-CI for x720
	CHANNEL_PARAMETER(	uDppOptBaseline,	"CI_SEL_BASELINE",		"0"					);

	COMMON_PARAMETER(	uDualTrace,			"DUAL_TRACE",			"NO"				);
	COMMON_PARAMETER(	uTimeHistoMax,		"TIME_HISTO_MAX",		"0"					);
	COMMON_PARAMETER(	uDTCTimeInterval,	"DTC_TIME_INTERVAL",	"0"					);
	COMMON_PARAMETER(	uDTCEventInterval,	"DTC_EVENT_INTERVAL",	"0"					);
	COMMON_PARAMETER(	uEminSkim,			"EMIN_SKIM",			"0"					);
	COMMON_PARAMETER(	uEmaxSkim,			"EMAX_SKIM",			"0"					);
	COMMON_PARAMETER(	uEnableEnergySkim,	"ENABLE_ENERGY_SKIM",	"NO"				);
	CHANNEL_PARAMETER(	uStopTime,			"STOP_TIME",			"0"					);

	COMMON_PARAMETER(	uGnuplotPath,		"GNUPLOT_PATH",			"./"				);

	COMMON_PARAMETER(	uVirtualProbeA,		"VIRTUAL_PROBE_A",		"0"					);
	COMMON_PARAMETER(	uVirtualProbeD1,	"VIRTUAL_PROBE_D1",		"0"					);
	COMMON_PARAMETER(	uVirtualProbeD2,	"VIRTUAL_PROBE_D2",		"0"					);

	COMMON_PARAMETER(	uVirtualProbeA1En,	"PLOT_TRACE_1_ENABLED",	"YES"				);
	COMMON_PARAMETER(	uVirtualProbeA1Gain,"PLOT_TRACE_1_GAIN",	"1"					);
	COMMON_PARAMETER(	uVirtualProbeA1Off,	"PLOT_TRACE_1_OFFSET",	"0"					);
	COMMON_PARAMETER(	uVirtualProbeA2En,	"PLOT_TRACE_2_ENABLED",	"YES"				);
	COMMON_PARAMETER(	uVirtualProbeA2Gain,"PLOT_TRACE_2_GAIN",	"1"					);
	COMMON_PARAMETER(	uVirtualProbeA2Off,	"PLOT_TRACE_2_OFFSET",	"0"					);
	COMMON_PARAMETER(	uVirtualProbeDEn,	"PLOT_TRACE_3_ENABLED",	"YES"				);
	COMMON_PARAMETER(	uVirtualProbeDGain,	"PLOT_TRACE_3_GAIN",	"100"				);
	COMMON_PARAMETER(	uVirtualProbeDOff,	"PLOT_TRACE_3_OFFSET",	"200"				);
	COMMON_PARAMETER(	uVirtualProbeD2En,	"PLOT_TRACE_4_ENABLED",	"YES"				);
	COMMON_PARAMETER(	uVirtualProbeD2Gain,"PLOT_TRACE_4_GAIN",	"100"				);
	COMMON_PARAMETER(	uVirtualProbeD2Off,	"PLOT_TRACE_4_OFFSET",	"200"				);

	CHANNEL_PARAMETER(	uCalibrationX1,		"CALIBRATION_X1",		"0"					);
	CHANNEL_PARAMETER(	uCalibrationX2,		"CALIBRATION_X2",		"0"					);
	CHANNEL_PARAMETER(	uCalibrationY1,		"CALIBRATION_Y1",		"0.0"				);
	CHANNEL_PARAMETER(	uCalibrationY2,		"CALIBRATION_Y2",		"0.0"				);

	COMMON_PARAMETER(	uDumpFileName,		"DUMPFILE",				"dump"				);
	COMMON_PARAMETER(	uDumpWaveforms,		"DUMP_WAVEFORMS",		"NO"				);
	COMMON_PARAMETER(	uDumpList,			"DUMP_LIST",			"NO"				);
	COMMON_PARAMETER(	uDumpEHisto,		"DUMP_EHISTO",			"NO"				);
	COMMON_PARAMETER(	uDumpTHisto,		"DUMP_THISTO",			"NO"				);

	COMMON_PARAMETER(	uPlotFps,			"PLOT_FPS",				"1.5"					);
    CHANNEL_PARAMETER(  uChargeSens,		"CI_SEL_CHARGE_SENS",	"0"					);
    COMMON_PARAMETER(	uListOutMode,		"LIST_OUT_MODE",		"ASCII"				);
)


#endif