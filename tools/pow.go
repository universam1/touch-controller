package main

import (
	"fmt"
	"math"
	"net/http"

	"github.com/go-echarts/go-echarts/v2/charts"
	"github.com/go-echarts/go-echarts/v2/opts"
	"github.com/go-echarts/go-echarts/v2/types"
)

const MAX_PWM_VALUE = 4096
const resolution = 1024

func main() {
	line := charts.NewLine()
	line.SetGlobalOptions(
		charts.WithInitializationOpts(opts.Initialization{Theme: types.ThemeWesteros}),
		charts.WithTitleOpts(opts.Title{Title: "Exponential functions"}),
		charts.WithTooltipOpts(opts.Tooltip{Trigger: "axis", Show: true}),
		charts.WithLegendOpts(opts.Legend{Show: true, Orient: "vertical", X: "left", Y: "center", Left: "10%"}),
		charts.WithXAxisOpts(opts.XAxis{Type: "value", Min: 0, Max: resolution}),
		charts.WithDataZoomOpts(opts.DataZoom{Type: "inside"}),
	)
	for bauchung := 1.0; bauchung > 1e-15; bauchung /= 10 {
		s, x := generateLineItems(bauchung, resolution)
		line.AddSeries(fmt.Sprintf("b: %.e exp: %.4f", bauchung, x), s)
	}
	http.HandleFunc("/", func(rw http.ResponseWriter, r *http.Request) {
		line.Render(rw)
	})
	http.ListenAndServe(":8081", nil)
}

func findPowerForCurve(b float64, resolution int) float64 {
	var exp float64 = 1
	var expLast float64 = 0
	var expadj float64 = 1
	var v float64
	i := 0
	for exp != expLast {
		i++
		expLast = exp
		v = math.Pow(float64(resolution), exp) * b
		if v > MAX_PWM_VALUE {
			exp -= expadj
			expadj = expadj / 10
		}
		exp += expadj
	}
	fmt.Printf("For %.e use %.15f - took %d\n", b, exp, i)

	return exp
}

func generateLineItems(bauch float64, resolution int) ([]opts.LineData, float64) {
	exp := findPowerForCurve(bauch, resolution)
	items := make([]opts.LineData, resolution)
	for x := 0; x < resolution; x++ {
		y := math.Pow(float64(x), exp) * bauch
		items[x] = opts.LineData{
			Value:  []interface{}{x, y},
			Symbol: "none",
		}
	}
	return items, exp
}
