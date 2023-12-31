// Package vadpcm provides tools for working with VADPCM-encoded audio.
package vadpcm

// #include "codec/vadpcm.h"
// #include <stdlib.h>
import "C"

import (
	"errors"
	"fmt"
	"strconv"
	"unsafe"
)

// CompressionType is the four-character code used to represent VADPCM
// compression.
const CompressionType = "VAPC"

// CompressionName is the descriptive name used to represent VADPCM compression.
const CompressionName = "VADPCM ~4-1"

// FrameSampleCount is the number of samples in a VADPCM frame.
const FrameSampleCount = C.kVADPCMFrameSampleCount

// FrameByteSize is the number of bytes in an encoded VADPCM frame.
const FrameByteSize = C.kVADPCMFrameByteSize

// MaxOrder maximum supported predictor order. Do not change this value. This is
// chosen to make the decoder state the same size as a 128-bit vector.
const MaxOrder = C.kVADPCMMaxOrder

// MaxPredictorCount is the maximum supported number of predictors. This is
// limited by the format of VADPCM frames, which only use four bits to encode
// the predictor index.
const MaxPredictorCount = C.kVADPCMMaxPredictorCount

// EncodeOrder is the predictor order when encoding. Other values are not supported. Do not
// change this value.
const EncodeOrder = C.kVADPCMEncodeOrder

// VectorSize is the number of elements in a VADPCM vector.
const VectorSize = 8

type vadpcmerr uint32

var errtext = [...]string{
	C.kVADPCMErrInvalidData:         "invalid data",
	C.kVADPCMErrLargeOrder:          "predictor order too large",
	C.kVADPCMErrLargePredictorCount: "predictor count too large",
	C.kVADPCMErrUnknownVersion:      "unknown VADPCM version",
	C.kVADPCMErrInvalidParams:       "invalid encoding parameters",
}

func (e vadpcmerr) Error() (s string) {
	if e < vadpcmerr(len(errtext)) {
		s = errtext[e]
	}
	return "vadpcmerr(" + strconv.Itoa(int(e)) + ")"
}

type Vector [VectorSize]int16

type Codebook struct {
	Order          int
	PredictorCount int
	Vectors        []Vector
}

// Decode decodes VADPCM-encoded audio data. Returns the number of frames
// decoded.
func Decode(codebook *Codebook, state *Vector, dest []int16, src []byte) (int, error) {
	order := codebook.Order
	predictor_count := codebook.PredictorCount
	if order < 0 || MaxOrder < order {
		return 0, fmt.Errorf("codebook has invalid order: %d", order)
	}
	if predictor_count < 0 {
		return 0, fmt.Errorf("codebook has invalid predictor count: %d", predictor_count)
	}
	if predictor_count > MaxPredictorCount {
		predictor_count = MaxPredictorCount
	}
	nvec := order * predictor_count
	vectors := codebook.Vectors
	if len(vectors) < nvec {
		return 0, errors.New("codebook has incorrect number of vectors")
	}

	dframes := len(dest) / FrameSampleCount
	sframes := len(src) / FrameByteSize
	nframes := dframes
	if sframes < nframes {
		nframes = sframes
	}

	err := C.vadpcm_decode(
		C.int(codebook.PredictorCount),
		C.int(codebook.Order),
		(*C.struct_vadpcm_vector)(unsafe.Pointer(&vectors[0])),
		(*C.struct_vadpcm_vector)(unsafe.Pointer(state)),
		C.size_t(nframes),
		(*C.int16_t)(unsafe.Pointer(&dest[0])),
		unsafe.Pointer(&src[0]))
	if err != 0 {
		return 0, vadpcmerr(err)
	}
	return nframes, nil
}

// Parameters contains the parameters for encoding,
type Parameters struct {
	PredictorCount int
}

// A Stats contains the statistics about how audio was encoded.
type Stats struct {
	SignalMeanSquare float64
	ErrorMeanSquare  float64
}

// Encode encodes audio as VADPCM.
func Encode(params *Parameters, data []int16) (*Codebook, []byte, *Stats, error) {
	predictor_count := params.PredictorCount
	if predictor_count < 0 {
		return nil, nil, nil, fmt.Errorf("invalid predictor count: %d", predictor_count)
	}
	if MaxPredictorCount < predictor_count {
		return nil, nil, nil, fmt.Errorf("predictor count is too large: %d", predictor_count)
	}
	nframes := len(data) / FrameSampleCount
	nvec := predictor_count * EncodeOrder
	if nframes == 0 {
		return &Codebook{
			Order:          EncodeOrder,
			PredictorCount: predictor_count,
			Vectors:        make([]Vector, nvec),
		}, nil, &Stats{}, nil
	}
	if predictor_count == 0 {
		return nil, nil, nil, errors.New("predictor count is zero")
	}
	cparams := C.struct_vadpcm_params{
		predictor_count: C.int(predictor_count),
	}
	vecs := make([]Vector, nvec)
	dest := make([]byte, nframes*FrameByteSize)
	var stats C.struct_vadpcm_stats
	err := C.vadpcm_encode(
		&cparams,
		(*C.struct_vadpcm_vector)(unsafe.Pointer(&vecs[0])),
		C.size_t(nframes),
		unsafe.Pointer(&dest[0]),
		(*C.int16_t)(unsafe.Pointer(&data[0])),
		&stats)
	if err != 0 {
		return nil, nil, nil, vadpcmerr(err)
	}
	return &Codebook{
			Order:          EncodeOrder,
			PredictorCount: predictor_count,
			Vectors:        vecs,
		}, dest, &Stats{
			SignalMeanSquare: float64(stats.signal_mean_square),
			ErrorMeanSquare:  float64(stats.error_mean_square),
		}, nil
}
