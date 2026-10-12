# Sample repository contract

`sample_repository` supplies a `write(context, sample)` callback. Return nonzero only when the sample has been accepted by the adapter. Return zero on a failed write; `monitor_accept` returns -1 and preserves its previous monitoring state. The included adapter writes CSV to stdout and stops on failure.

For a future database, model device identity, boot/session identity, sequence, uptime, received-at time, temperature, humidity, presence and validity. A sequence alone is not globally unique because it resets on reboot and wraps. Add a gateway session identifier or an explicit device boot identifier before enforcing uniqueness. Store invalid readings with their validity flag, or record them in a separate fault stream.

Use a bounded queue if database latency is greater than the acquisition interval. Specify retry, duplicate and overflow policies explicitly. Temperature and humidity are scaled integer values, rather than floating-point storage requirements. No database client or schema migration is installed.
