// Copyright 2026 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#pragma once

typedef enum {
    kExtUnknown,
    kExtAIFF,
    kExtAIFC,
    kExtWAVE,
} extension;

// Figure out the filetype for a file, if we can.
extension extension_for_file(const char *filename);

// Return the name for a file extension.
const char *name_for_extension(extension ext);
