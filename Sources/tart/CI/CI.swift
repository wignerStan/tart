struct CI {
  private static let rawVersion = "${VERSION}"

  static var version: String {
    // The guest agent discovers Tart through a console device name and requires
    // semantic versioning. SNAPSHOT makes it reject even a valid Tart VM.
    rawVersion.expanded() ? rawVersion : "2.40.1-vmnet-dev"
  }

  static var release: String? {
    rawVersion.expanded() ? "tart@\(rawVersion)" : nil
  }
}

private extension String {
  func expanded() -> Bool {
    !isEmpty && !starts(with: "$")
  }
}
