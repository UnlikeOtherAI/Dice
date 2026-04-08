import UIKit
import SwiftUI

/// UIViewRepresentable that owns and drives a DiceView.
struct DiceGameView: UIViewRepresentable {
    let sides: Int
    let hue: Double   // 0.0 – 1.0, maps to rainbow
    let whiteNumbers: Bool
    let zoom: Double
    let rollTrigger: Int  // increment to trigger a roll
    let onRoll: (Int) -> Void

    func makeUIView(context: Context) -> DiceView {
        let view = DiceView()
        view.backgroundColor = .white
        view.startRenderLoop()
        view.renderer.setCameraDistance(cameraDistance(for: sides, zoom: zoom))

        let die = view.renderer.addDie(
            withSides: Int32(sides),
            bevel: 0.05,
            dieColor: UIColor(hue: CGFloat(hue), saturation: 0.85, brightness: 0.95, alpha: 1),
            whiteNumbers: whiteNumbers
        )
        context.coordinator.dieHandle = die

        let tap = UITapGestureRecognizer(target: context.coordinator,
                                         action: #selector(Coordinator.handleTap))
        view.addGestureRecognizer(tap)
        context.coordinator.diceView = view
        context.coordinator.onRoll = onRoll

        return view
    }

    func updateUIView(_ uiView: DiceView, context: Context) {
        let coord = context.coordinator
        let newColor = UIColor(hue: CGFloat(hue), saturation: 0.85, brightness: 0.95, alpha: 1)
        uiView.renderer.setCameraDistance(cameraDistance(for: sides, zoom: zoom))
        coord.onRoll = onRoll

        if coord.currentSides != sides {
            if let old = coord.dieHandle {
                uiView.renderer.removeDie(old)
            }
            let die = uiView.renderer.addDie(
                withSides: Int32(sides),
                bevel: 0.05,
                dieColor: newColor,
                whiteNumbers: whiteNumbers
            )
            coord.dieHandle = die
            coord.currentSides = sides
            coord.currentHue = hue
            coord.currentWhiteNumbers = whiteNumbers
        } else if coord.currentHue != hue || coord.currentWhiteNumbers != whiteNumbers {
            if let old = coord.dieHandle {
                uiView.renderer.removeDie(old)
            }
            let die = uiView.renderer.addDie(
                withSides: Int32(sides),
                bevel: 0.05,
                dieColor: newColor,
                whiteNumbers: whiteNumbers
            )
            coord.dieHandle = die
            coord.currentHue = hue
            coord.currentWhiteNumbers = whiteNumbers
        }

        if coord.lastRollTrigger != rollTrigger, let handle = coord.dieHandle {
            let maxFace = sides
            let result = Int.random(in: 1...maxFace)
            uiView.renderer.rollDie(handle, result: Int32(result), duration: 2.5)
            onRoll(result)
            coord.lastRollTrigger = rollTrigger
        }
    }

    func makeCoordinator() -> Coordinator {
        Coordinator(sides: sides, hue: hue, whiteNumbers: whiteNumbers)
    }

    private func cameraDistance(for sides: Int, zoom: Double) -> Float {
        let baseDistance: Double
        switch sides {
        case 4: baseDistance = 15.0
        case 6: baseDistance = 14.0
        case 8: baseDistance = 12.5
        case 10: baseDistance = 15.0
        case 12: baseDistance = 14.5
        case 16: baseDistance = 11.5
        case 20: baseDistance = 15.0
        case 32: baseDistance = 15.0
        default: baseDistance = 15.0
        }
        return Float(baseDistance / max(zoom, 0.25))
    }

    class Coordinator: NSObject {
        var diceView: DiceView?
        var dieHandle: UInt32?
        var currentSides: Int
        var currentHue: Double
        var currentWhiteNumbers: Bool
        var lastRollTrigger: Int = 0
        var onRoll: ((Int) -> Void)?

        init(sides: Int, hue: Double, whiteNumbers: Bool) {
            self.currentSides = sides
            self.currentHue = hue
            self.currentWhiteNumbers = whiteNumbers
        }

        @objc func handleTap() {
            guard let view = diceView, let handle = dieHandle else { return }
            let result = Int.random(in: 1...currentSides)
            view.renderer.rollDie(handle, result: Int32(result), duration: 2.5)
            onRoll?(result)
        }
    }
}
