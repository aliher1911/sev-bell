# Sev Bell
sketch for servo control:
- when '0', '\n' is received abort current sequence if active
- when '1', '\n' is received on serial play sequence of movements {pos, delay}, {pos, delay}
- when '2', &lt;message&gt;, '\n' is received decode message and change the sequence to provided one
- start with predefined sequence for single bell: left, right, middle
- start with setting servo to the middle

