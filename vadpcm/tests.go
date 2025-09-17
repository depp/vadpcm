package main

import (
	"encoding/csv"
	"errors"
	"log/slog"
	"math"
	"os"
	"strconv"
	"sync/atomic"

	"github.com/depp/vadpcm/lib/parallel"
	"github.com/depp/vadpcm/lib/vadpcm"

	"github.com/spf13/cobra"
)

func getStats(file string) (*vadpcm.Stats, error) {
	ad, err := readAudio(file)
	if err != nil {
		return nil, err
	}
	ad.samples = padAudio(ad.samples)
	_, _, stats, err := vadpcm.Encode(&vadpcm.Parameters{
		PredictorCount: flagPredictorCount,
	}, ad.samples)
	return stats, err
}

var cmdTestStats = cobra.Command{
	Use:   "stats",
	Short: "Collect encoding performance stats",
	Args:  cobra.MinimumNArgs(1),
	RunE: func(_ *cobra.Command, args []string) error {
		stats := make([]*vadpcm.Stats, len(args))
		var errcount uint64
		parallel.Do(len(args), func(i int) {
			s, err := getStats(args[i])
			if err != nil {
				atomic.AddUint64(&errcount, 1)
				slog.Error(err.Error())
				return
			}
			stats[i] = s
		})
		w := csv.NewWriter(os.Stdout)
		w.Write([]string{"File", "SNR dB"})
		for i, st := range stats {
			if st != nil {
				e := "None"
				if st.ErrorMeanSquare > 0.0 {
					e = strconv.FormatFloat(10*math.Log10(st.SignalMeanSquare/st.ErrorMeanSquare), 'f', 2, 64)
				}
				w.Write([]string{args[i], e})
			}
		}
		w.Flush()
		if err := w.Error(); err != nil {
			return err
		}
		if errcount > 0 {
			return errors.New("encountered errors during test")
		}
		return nil
	},
}

var cmdTest = cobra.Command{
	Use:   "test",
	Short: "Run codec tests",
}
