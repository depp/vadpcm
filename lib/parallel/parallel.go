package parallel

import (
	"runtime"
	"sync"
	"sync/atomic"
)

// Do calls the given function in parallel n times.
func Do(n int, f func(i int)) {
	jobs := runtime.NumCPU()
	if jobs > n {
		jobs = n
	}
	if jobs <= 1 {
		for i := 0; i < n; i++ {
			f(i)
		}
		return
	}
	var counter uint64
	var wg sync.WaitGroup
	wg.Add(jobs)
	for i := 0; i < jobs; i++ {
		go func() {
			defer wg.Done()
			for {
				v := atomic.AddUint64(&counter, 1) - 1
				if v >= uint64(n) {
					break
				}
				f(int(v))
			}
		}()
	}
	wg.Wait()
}
