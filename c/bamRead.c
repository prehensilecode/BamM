//#############################################################################
//
//   bamRead.c
//
//   Data structures for representing mapped reads in a BAM file
//
//   Copyright (C) Michael Imelfort
//
//   This library is free software; you can redistribute it and/or
//   modify it under the terms of the GNU Lesser General Public
//   License as published by the Free Software Foundation; either
//   version 3.0 of the License, or (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public
//   License along with this library.
//
//#############################################################################

// system includes
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <libgen.h>

// cfuhash
#include "cfuhash.h"

// local includes
#include "bamRead.h"

/*
 * convert mapping information to a readable string
 * SEE: bamRead.h for deets
 */

static const char MITEXT[6][2][12] = {{"p_PR_PM_UG;\0",   // FIR, U
                                       "p_PR_PM_PG;\0"},  // FIR, P
                                      {"p_PR_PM_UG;\0",  // SEC, U
                                       "p_PR_PM_PG;\0"},  // SEC, P
                                      {"p_PR_UM_NG;\0",  // SNGL_FIR, U
                                       "p_PR_EM_NG;\0"},  // SNGL_FIR, P
                                      {"p_PR_UM_NG;\0",  // SNGL_SEC, U
                                       "p_PR_EM_NG;\0"},  // SNGL_SEC, P
                                      {"p_UR_NM_NG;\0",  // SNGL, U
                                       "p_UR_EM_NG;\0"},  // SNGL, P
                                      {"p_ER_NM_NG;\0",  // ERR, U
                                       "p_ER_NM_NG;\0"}}; // ERR, P

void getMITEXT(int rpi, int paired, char * buffer) {
    int i = 0;
    for(;i<12;++i) {
        buffer[i] = MITEXT[rpi][paired][i];
    }
}

BM_mappedRead * makeMappedRead(char * seqId,
                                char * seq,
                                char * qual,
                                uint16_t idLen,
                                uint16_t seqLen,
                                uint16_t qualLen,
                                uint8_t rpi,
                                uint16_t group,
                                BM_mappedRead * prev_MR) {

    BM_mappedRead * MR = calloc(1, sizeof(BM_mappedRead));
    MR->seqId = strdup(seqId);
    if( 0 != seq ) {
        MR->seq = strdup(seq);
    }
    else {
        MR->seq = 0;
    }
    MR->rpi = rpi;
    MR->idLen = idLen;
    MR->seqLen = seqLen;
    MR->qualLen = qualLen;
    MR->group = group;
    if( 0 != qual )
        MR->qual = strdup(qual);
    else
        MR->qual = 0;

    if(prev_MR)
        prev_MR->nextRead = MR;

    MR->nextRead = 0;
    MR->partnerRead = 0;
    MR->nextPrintingRead = 0;

    return MR;
}

BM_mappedRead * getNextMappedRead(BM_mappedRead * MR) {
    return MR->nextRead;
}

BM_mappedRead * getNextPrintRead(BM_mappedRead * MR) {
    return MR->nextPrintingRead;
}

void setNextPrintRead(BM_mappedRead * baseMR, BM_mappedRead * nextMR) {
    baseMR->nextPrintingRead = nextMR;
}

BM_mappedRead * getPartner(BM_mappedRead * MR) {
    if(MR->partnerRead)
        return MR->partnerRead;
    fprintf(stdout, "1. nein partner %d\n", MR->rpi);
    return (BM_mappedRead *)0;
}

int partnerInSameGroup(BM_mappedRead * MR) {
    if(MR->partnerRead)
        return ((MR->partnerRead)->group == MR->group);
    fprintf(stdout, "2. nein partner %d\n", MR->rpi);
    return 0;
}

void destroyMappedReads(BM_mappedRead * root_MR) {
    BM_mappedRead * tmp_MR = 0;
    while(root_MR) {
        if(root_MR->seqId) free(root_MR->seqId);
        if(root_MR->seq) free(root_MR->seq);
        if(root_MR->qual) free(root_MR->qual);
        tmp_MR = root_MR->nextRead;
        free(root_MR);
        root_MR = tmp_MR;
    }
}

void destroyPrintChain(BM_mappedRead * root_MR) {
    BM_mappedRead * tmp_MR = 0;
    while(root_MR) {
        if(root_MR->seqId) free(root_MR->seqId);
        if(root_MR->seq) free(root_MR->seq);
        if(root_MR->qual) free(root_MR->qual);
        tmp_MR = root_MR->nextPrintingRead;
        free(root_MR);
        root_MR = tmp_MR;
    }
}

void printMappedRead(BM_mappedRead * MR,
                     FILE * f,
                     char * groupName,
                     int headerOnly,
                     int pairedOutput) {
    if(0 == f) { f = stdout; }

    char MI_buffer[12] = "";
    getMITEXT(MR->rpi, pairedOutput, MI_buffer);

    if(headerOnly) {
        fprintf(f,
                HEADER_FORMAT,
                groupName,
                MI_buffer,
                MR->seqId);
    }
    else {
        if(MR->qual) {
            fprintf(f,
                    FASTQ_FORMAT,
                    groupName,
                    MI_buffer,
                    MR->seqId,
                    MR->seq,
                    MR->qual);
        }
        else {
            fprintf(f,
                    FASTA_FORMAT,
                    groupName,
                    MI_buffer,
                    MR->seqId,
                    MR->seq);
        }
    }
}

void sprintMappedRead(BM_mappedRead * MR,
                      char * buffer,
                      int * count,
                      char * groupName,
                      int headerOnly,
                      int pairedOutput) {
    char MI_buffer[12] = "";
    getMITEXT(MR->rpi, pairedOutput, MI_buffer);

    if(headerOnly) {
        *count = sprintf(buffer,
                         HEADER_FORMAT,
                         groupName,
                         MI_buffer,
                         MR->seqId);
    }
    else {
        if(MR->qual) {
            *count = sprintf(buffer,
                             FASTQ_FORMAT,
                             groupName,
                             MI_buffer,
                             MR->seqId,
                             MR->seq,
                             MR->qual);
        }
        else {
            *count = sprintf(buffer,
                             FASTA_FORMAT,
                             groupName,
                             MI_buffer,
                             MR->seqId,
                             MR->seq);
        }
    }
}

void printMappedReads(BM_mappedRead * root_MR,
                      FILE * f,
                      char ** groupNames,
                      int headersOnly,
                      int pairedOutput) {

    if(0 == f) { f = stdout; }

    char MI_buffer[12] = "";

    BM_mappedRead * MR = root_MR;
    if(headersOnly) {
        while(MR) {
            getMITEXT(MR->rpi, pairedOutput, MI_buffer);
            fprintf(f,
                    HEADER_FORMAT,
                    groupNames[MR->group],
                    MI_buffer,
                    MR->seqId);
            MR = MR->nextRead;
        }
    }
    else {
        if(MR->qual) {
            while(MR) {
                getMITEXT(MR->rpi, pairedOutput, MI_buffer);
                fprintf(f,
                        FASTQ_FORMAT,
                        groupNames[MR->group],
                        MI_buffer,
                        MR->seqId,
                        MR->seq,
                        MR->qual);
                MR = MR->nextRead;
            }
        }
        else {
            while(MR) {
                getMITEXT(MR->rpi, pairedOutput, MI_buffer);
                fprintf(f,
                        FASTA_FORMAT,
                        groupNames[MR->group],
                        MI_buffer,
                        MR->seqId,
                        MR->seq);
                MR = MR->nextRead;
            }
        }
    }
}
