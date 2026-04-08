import UIKit
import SwiftUI

/// UIViewRepresentable that owns and drives a DiceView.
struct DiceGameView: UIViewRepresentable {
    let sides: Int
    let hue: Double   // 0.0 – 1.0, maps to rainbow
    let rollTrigger: Int  // increment to trigger a roll

    func makeUIView(context: Context) -> DiceView {
        let view = DiceView()
        view.backgroundColor = .clear
        view.startRenderLoop()

        let die = view.renderer.addDie(
            withSides: Int32(sides),
            bevel: 0.05,
            dieColor: UIColor(hue: CGFloat(hue), saturation: 0.85, brightness: 0.95, alpha: 1),
            whiteNumbers: true
        )
        context.coordinator.dieHandle = die

        // Tap → roll
        let tap = UITapGestureRecognizer(target: context.coordinator,
                                         action: #selector(Coordinator.handleTap))
        view.addGestureRecognizer(tap)
        context.coordinator.diceView = view

        return view
    }

    func updateUIView(_ uiView: DiceView, context: Context) {
        let coord = context.coordinator
        let newColor = UIColor(hue: CGFloat(hue), saturation: 0.85, brightness: 0.95, alpha: 1)

        // Re-add die if sides changed
        if coord.currentSides != sides {
            if let old = coord.dieHandle {
                uiView.renderer.removeDie(old)
            }
            let die = uiView.renderer.addDie(
                withSides: Int32(sides),
                bevel: 0.05,
                dieColor: newColor,
                whiteNumbers: true
            )
            coord.dieHandle = die
            coord.currentSides = sides
            coord.currentHue = hue
        } else if coord.currentHue != hue {
            // Color changed: remove and re-add (simplest path for colour update)
            if let old = coord.dieHandle {
                uiView.renderer.removeDie(old)
            }
            let die = uiView.renderer.addDie(
                withSides: Int32(sides),
                bevel: 0.05,
                dieColor: newColor,
                whiteNumbers: true
            )
            coord.dieHandle = die
            coord.currentHue = hue
        }

        // Roll triggered
        if coord.lastRollTrigger != rollTrigger, let handle = coord.dieHandle {
            let maxFace = sides
            let result = Int32.random(in: 1...Int32(maxFace))
            uiView.renderer.rollDie(handle, result: result, duration: 2.5)
            coord.lastRollTrigger = rollTrigger
        }
    }

    func makeCoordinator() -> Coordinator { Coordinator(sides: sides, hue: hue) }

    class Coordinator: NSObject {
        var diceView: DiceView?
        var dieHandle: UInt32?
        var currentSides: Int
        var currentHue: Double
        var lastRollTrigger: Int = 0

        init(sides: Int, hue: Double) {
            self.currentSides = sides
            self.currentHue = hue
        }

        @objc func handleTap() {
            guard let view = diceView, let handle = dieHandle else { return }
            let result = Int32.random(in: 1...Int32(currentSides))
            view.renderer.rollDie(handle, result: result, duration: 2.5)
        }
    }
}
