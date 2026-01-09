// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once

#include "common/util.h"

#define AIFF_CKID FOURCC('F', 'O', 'R', 'M')
#define AIFF_KIND FOURCC('A', 'I', 'F', 'F')
#define AIFC_KIND FOURCC('A', 'I', 'F', 'C')

#define CHUNK_COMM FOURCC('C', 'O', 'M', 'M')
#define CHUNK_FVER FOURCC('F', 'V', 'E', 'R')
#define CHUNK_SSND FOURCC('S', 'S', 'N', 'D')
#define CHUNK_MARK FOURCC('M', 'A', 'R', 'K')
#define CHUNK_INST FOURCC('I', 'N', 'S', 'T')
#define CHUNK_APPL FOURCC('A', 'P', 'P', 'L')

#define CODEC_PCM FOURCC('N', 'O', 'N', 'E')
#define CODEC_VADPCM FOURCC('V', 'A', 'P', 'C')
