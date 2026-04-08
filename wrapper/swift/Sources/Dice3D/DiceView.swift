import UIKit

// Re-export DiceView from the platform layer for Swift callers.
// The platform/ios/DiceView.h is bridged via the app's bridging header.
// This file documents the expected API contract.

// DiceView is a UIView subclass exposed from Obj-C++.
// Usage:
//   let diceView = DiceView(frame: view.bounds)
//   view.addSubview(diceView)
//   diceView.startRenderLoop()
//   let ctrl = DiceController(view: diceView)
//   let handle = ctrl.addDie(sides: 20)
//   ctrl.roll(handle: handle, result: 15)
