import UIKit

// Import the Obj-C++ DiceRenderer and DiceView from platform/ios/
// (linked at app level, not via SPM — the binary is platform-native)

/// Swift convenience wrapper over DiceRenderer.
@objc public class DiceController: NSObject {
    @objc public let view: DiceView

    @objc public init(view: DiceView) {
        self.view = view
        super.init()
    }

    /// Add a die and return its handle.
    @objc @discardableResult
    public func addDie(sides: Int, color: UIColor = .red, whiteNumbers: Bool = true) -> UInt32 {
        return view.renderer.addDie(
            withSides: Int32(sides),
            bevel: 0.05,
            dieColor: color,
            whiteNumbers: whiteNumbers
        )
    }

    /// Remove a die by handle.
    @objc public func removeDie(handle: UInt32) {
        view.renderer.removeDie(handle)
    }

    /// Roll a die to a specific result over `duration` seconds.
    @objc public func roll(handle: UInt32, result: Int, duration: Float = 2.0) {
        view.renderer.rollDie(handle, result: Int32(result), duration: duration)
    }
}
