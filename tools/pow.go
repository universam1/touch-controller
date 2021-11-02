package main

import (
	"fmt"
	"math"
)

//----------------------------------------------------------------------------------------------------
// void findMaxForPow(uint16_t maxInputValue) {
var exp float64 = 1
var expLast float64 = 0
var expadj float64 = 1
var v float64 = 0

// 	while (exp != expLast) {
// 	  expLast = exp;
// 	  v = pow(maxInputValue, exp);
// 	  if (v > MAX_PWM_VALUE) {
// 		exp -= expadj;
// 		expadj = expadj / 10;
// 	  }
// 	  exp += expadj;
// 	}
// 	Serial.print(F("For a maximum input value of: "));
// 	Serial.print(maxInputValue);
// 	Serial.print(F("  Use: "));
// 	Serial.print(exp, 10);
// 	Serial.println(F("  In pow(..) function "));
//   }

const MAX_PWM_VALUE = 4096

func main() {
	for maxInputValue := 100; maxInputValue <= 100; maxInputValue++ {
		// go func(maxInputValue int) {
		var exp float64 = 1
		var expLast float64 = 0
		var expadj float64 = 1
		var v float64
		i := 0
		for exp != expLast {
			i++
			expLast = exp
			v = math.Pow(float64(maxInputValue), exp)
			if v > MAX_PWM_VALUE {
				exp -= expadj
				expadj = expadj / 10
			}
			exp += expadj
		}
		fmt.Printf("For %d use %.15f - took %d\n", maxInputValue, exp, i)
		// }(maxInputValue)
	}
}
