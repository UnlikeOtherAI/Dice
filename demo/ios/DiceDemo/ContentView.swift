import SwiftUI

private let dieTypes = [4, 6, 8, 10, 12, 16, 20, 32]
private let settingsStore = UserDefaults.standard
private let defaultHueLevels: [Int: Double] = [
    4: 0.60,
    6: 0.60,
    8: 0.60,
    10: 0.60,
    12: 0.60,
    16: 0.60,
    20: 0.60,
    32: 0.60,
]
private let defaultWhiteNumberLevels: [Int: Bool] = [
    4: true,
    6: true,
    8: true,
    10: true,
    12: true,
    16: true,
    20: true,
    32: true,
]
private let defaultZoomLevels: [Int: Double] = [
    4: 1.00,
    6: 1.05,
    8: 1.15,
    10: 1.00,
    12: 1.00,
    16: 1.20,
    20: 1.00,
    32: 1.00,
]

private enum DieSettingsStore {
    static func hue(for sides: Int) -> Double {
        let key = "dice.settings.hue.\(sides)"
        if settingsStore.object(forKey: key) == nil {
            return defaultHueLevels[sides] ?? 0.60
        }
        return settingsStore.double(forKey: key)
    }

    static func setHue(_ value: Double, for sides: Int) {
        settingsStore.set(value, forKey: "dice.settings.hue.\(sides)")
    }

    static func whiteNumbers(for sides: Int) -> Bool {
        let key = "dice.settings.whiteNumbers.\(sides)"
        if settingsStore.object(forKey: key) == nil {
            return defaultWhiteNumberLevels[sides] ?? true
        }
        return settingsStore.bool(forKey: key)
    }

    static func setWhiteNumbers(_ value: Bool, for sides: Int) {
        settingsStore.set(value, forKey: "dice.settings.whiteNumbers.\(sides)")
    }

    static func zoom(for sides: Int) -> Double {
        let key = "dice.settings.zoom.\(sides)"
        if settingsStore.object(forKey: key) == nil {
            return defaultZoomLevels[sides] ?? 1.00
        }
        return settingsStore.double(forKey: key)
    }

    static func setZoom(_ value: Double, for sides: Int) {
        settingsStore.set(value, forKey: "dice.settings.zoom.\(sides)")
    }

    static func loadHues() -> [Int: Double] {
        Dictionary(uniqueKeysWithValues: dieTypes.map { ($0, hue(for: $0)) })
    }

    static func loadWhiteNumbers() -> [Int: Bool] {
        Dictionary(uniqueKeysWithValues: dieTypes.map { ($0, whiteNumbers(for: $0)) })
    }

    static func loadZooms() -> [Int: Double] {
        Dictionary(uniqueKeysWithValues: dieTypes.map { ($0, zoom(for: $0)) })
    }
}

struct ContentView: View {
    @State private var selectedSides = 16
    @State private var hueLevels = DieSettingsStore.loadHues()
    @State private var whiteNumberLevels = DieSettingsStore.loadWhiteNumbers()
    @State private var rollTrigger = 1
    @State private var zoomLevels = DieSettingsStore.loadZooms()
    @State private var selectedResult: Int?

    var body: some View {
        ZStack {
            Color.white.ignoresSafeArea()

            VStack(spacing: 0) {
                DiceGameView(
                    sides: selectedSides,
                    hue: hueBinding.wrappedValue,
                    whiteNumbers: whiteNumbersBinding.wrappedValue,
                    zoom: zoomBinding.wrappedValue,
                    rollTrigger: rollTrigger
                ) { result in
                    selectedResult = result
                }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)

                controlPanel
            }
        }
    }

    private var controlPanel: some View {
        VStack(spacing: 16) {
            Picker("Die", selection: $selectedSides) {
                ForEach(dieTypes, id: \.self) { n in
                    Text("d\(n)").tag(n)
                }
            }
            .pickerStyle(.segmented)

            VStack(spacing: 6) {
                HStack {
                    Text("Color").foregroundStyle(.secondary)
                    Spacer()
                    RoundedRectangle(cornerRadius: 4)
                        .fill(Color(hue: hueBinding.wrappedValue, saturation: 0.85, brightness: 0.95))
                        .frame(width: 32, height: 20)
                }
                LinearGradientSlider(value: hueBinding)
            }

            HStack {
                Text("Black Numbers")
                    .foregroundStyle(.secondary)
                Spacer()
                Toggle("", isOn: blackNumbersBinding)
                .labelsHidden()
            }

            VStack(spacing: 6) {
                HStack {
                    Text("Zoom").foregroundStyle(.secondary)
                    Spacer()
                    Text("\(Int((zoomBinding.wrappedValue * 100).rounded()))%")
                        .foregroundStyle(.secondary)
                }
                Slider(value: zoomBinding, in: 0.7...1.5)
            }

            HStack {
                Text("Selected")
                    .foregroundStyle(.secondary)
                Spacer()
                Text(selectedResult.map(String.init) ?? "-")
                    .font(.headline)
                    .monospacedDigit()
            }

            Text("Tap the die to roll")
                .font(.caption)
                .foregroundStyle(.tertiary)
        }
        .padding(20)
        .background(.ultraThinMaterial)
    }

    private var zoomBinding: Binding<Double> {
        Binding(
            get: { zoomLevels[selectedSides] ?? 1.0 },
            set: {
                zoomLevels[selectedSides] = $0
                DieSettingsStore.setZoom($0, for: selectedSides)
            }
        )
    }

    private var hueBinding: Binding<Double> {
        Binding(
            get: { hueLevels[selectedSides] ?? 0.6 },
            set: {
                hueLevels[selectedSides] = $0
                DieSettingsStore.setHue($0, for: selectedSides)
            }
        )
    }

    private var whiteNumbersBinding: Binding<Bool> {
        Binding(
            get: { whiteNumberLevels[selectedSides] ?? true },
            set: {
                whiteNumberLevels[selectedSides] = $0
                DieSettingsStore.setWhiteNumbers($0, for: selectedSides)
            }
        )
    }

    private var blackNumbersBinding: Binding<Bool> {
        Binding(
            get: { !(whiteNumberLevels[selectedSides] ?? true) },
            set: { whiteNumberLevels[selectedSides] = !$0 }
        )
    }
}

// MARK: - Rainbow slider

struct LinearGradientSlider: View {
    @Binding var value: Double

    private let gradient = LinearGradient(
        colors: (0..<20).map { Color(hue: Double($0) / 20, saturation: 0.85, brightness: 0.95) },
        startPoint: .leading, endPoint: .trailing
    )

    var body: some View {
        GeometryReader { geo in
            ZStack(alignment: .leading) {
                RoundedRectangle(cornerRadius: 6).fill(gradient).frame(height: 24)
                Circle()
                    .fill(.white)
                    .shadow(radius: 3)
                    .frame(width: 28, height: 28)
                    .offset(x: CGFloat(value) * (geo.size.width - 28))
                    .gesture(
                        DragGesture(minimumDistance: 0)
                            .onChanged { drag in
                                let x = drag.location.x
                                value = min(max(Double(x / geo.size.width), 0), 1)
                            }
                    )
            }
        }
        .frame(height: 28)
    }
}
