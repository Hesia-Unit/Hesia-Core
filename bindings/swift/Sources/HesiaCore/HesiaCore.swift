import CHesiaCore

public enum HesiaCoreError: Error, CustomStringConvertible {
    case status(Int32, String)

    public var description: String {
        switch self {
        case let .status(code, message):
            return "HESIA Core status \(code): \(message)"
        }
    }
}

private func check(_ status: hesia_status_t, _ error: inout hesia_error_t) throws {
    if status == HESIA_STATUS_OK {
        return
    }
    let message = String(cString: hesia_error_message(&error))
    throw HesiaCoreError.status(status.rawValue, message)
}

public func version() -> String {
    String(cString: hesia_version())
}

public func featureAvailable(_ feature: hesia_feature_t) -> Bool {
    hesia_feature_available(feature) != 0
}

public func providerVersion(_ feature: hesia_feature_t) -> String {
    String(cString: hesia_feature_provider_version(feature))
}

public func randomBytes(count: Int) throws -> [UInt8] {
    var out = [UInt8](repeating: 0, count: count)
    var error = hesia_error_t()
    hesia_error_init(&error)
    let status = out.withUnsafeMutableBufferPointer { buffer in
        hesia_random(buffer.baseAddress, buffer.count, &error)
    }
    try check(status, &error)
    return out
}

public final class Watchdog {
    private var raw = hesia_watchdog_t()

    public init(timeoutMilliseconds: UInt64, maxMissed: UInt32 = 1) throws {
        var error = hesia_error_t()
        hesia_error_init(&error)
        let status = hesia_watchdog_init(&raw, timeoutMilliseconds, maxMissed, &error)
        try check(status, &error)
    }

    public func arm() throws {
        try call(hesia_watchdog_arm)
    }

    public func heartbeat() throws {
        try call(hesia_watchdog_heartbeat)
    }

    public func checkDeadline() throws {
        try call(hesia_watchdog_check)
    }

    public func disarm() {
        hesia_watchdog_disarm(&raw)
    }

    private func call(_ function: (UnsafeMutablePointer<hesia_watchdog_t>?, UnsafeMutablePointer<hesia_error_t>?) -> hesia_status_t) throws {
        var error = hesia_error_t()
        hesia_error_init(&error)
        let status = function(&raw, &error)
        try check(status, &error)
    }
}
