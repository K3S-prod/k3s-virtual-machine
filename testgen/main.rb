
$OPC = 0
$OPERANDS = 0

public def UpdateBC(*args)
  puts "Requested BC inst"
  puts "Frame is " + args[0].to_s
  puts "Img is " + args[1].to_s
  $OPC = 6 # Hardcoded Opcode::RET
  $OPERANDS = 0 
end
