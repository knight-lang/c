module Function
	FUNCTIONS = {
		?P => (OP_PROMPT = 0x01),
		?R => (OP_RANDOM = 0x02),

		?E => (OP_EVAL   = 0x20),
		?B => (OP_BLOCK  = 0x21),
		?C => (OP_CALL   = 0x22),
		?` => (OP_SYSTEM = 0x23),
		?Q => (OP_QUIT   = 0x24),
		?! => (OP_NOT    = 0x25),
		?L => (OP_LENGTH = 0x26),
		?D => (OP_DUMP   = 0x27),
		?O => (OP_OUTPUT = 0x28),
		
		?+ => (OP_ADD    = 0x40),
		?- => (OP_SUB    = 0x41),
		?* => (OP_MUL    = 0x42),
		?/ => (OP_DIV    = 0x43),
		?% => (OP_MOD    = 0x44),
		?^ => (OP_POW    = 0x45),
		?? => (OP_EQL    = 0x46),
		?< => (OP_LTH    = 0x47),
		?> => (OP_GTH    = 0x48),
		?& => (OP_AND    = 0x49),
		?| => (OP_OR     = 0x50),
		?; => (OP_THEN   = 0x51),
		?= => (OP_ASSIGN = 0x52),
		?W => (OP_WHILE  = 0x53),
		
		?I => (OP_IF  = 0x60),
		?G => (OP_GET = 0x61),

		?S => (OP_SET = 0x80),
	}

	def self.arity(func) func / 0x20 end
end

module Ast
	Immediate = Struct.new(:data)
	Variable = Struct.new(:name)
	Function = Struct.new(:func, :args)

	def self.parse!(s)
		s.slice! /\A(#[^\n]*|[\s(){}\[\]:])*/m

		case
		when s.slice!(/\A\d+/)                     then Immediate.new $&.to_i
		when s.slice!(/\A([TF])[A-Z_]*/)           then Immediate.new $1 == ?T
		when s.slice!(/\AN[A-Z_]*/)                then Immediate.new nil
		when s.slice!(/\A(?:'([^']*)'|"([^"]*)")/) then Immediate.new $+
		when s.slice!(/\A[a-z_][a-z0-9_]*/)        then Variable.new $&
		when s.slice!(/\A(.)(?:(?<=[A-Z])[A-Z]+)?/)
			Function.new $1, ::Function::FUNCTIONS.fetch($1).then { ::Function::arity _1 }.times.map { parse! s }
		else raise "unknown empty stream (#{s.inspect})"
		end
	end
end

class Block
	def initialize
		@locals =[]

pp Ast.parse! <<EOS
: = fact BLOCK
	: IF (< n 2)
		: n
		: * n (; = n - n 1 : CALL fact)
1
 2 3 4 5 6 7 8 9 10 11 1 1 1 1 1 1 1 23 1234 1235 12 3512 35 1235 123 515 
EOS
# class Value
# 	def initialize(data)
# 		@data = data
# 	end
# end

# class Frame
# 	def initialize
# 		@locals, @constants, @bytecode = {}, {}, []
# 	end

# 	def parse(stream)
# 		case
# 		when stream.slice!(/\A\d+/) then 
# 	end
# end

# # #include "frame.h"

# # static const char *stream;

# # /*
# # ; = fact BLOCK
# # 	: IF (< n 2)
# # 		: n
# # 		: * n (; = n - n 1 : CALL fact)
# # ; = n 10
# # : CALL fact

# # ===
# # fact:
# # (c0 = 2, c1 = 1)
# # 0.  t0 = 2
# # 1.  t1 = n
# # 2.  t2 = t1 < t0
# # 3.  if (!t2) goto 6
# # 4.  t3 = n
# # 5.  return t3
# # 6.  t4 = n
# # 7.  t5 = n - 1
# # 8.  n = t5
# # 9.  t6 = CALL fact
# # 10. return t4 * t6

# # */
# # struct {
# # 	struct {
		
# # 	};
# # 	unsigned nlocals;

# # 	nconstants, nopcodes;

# # 	kn_value *constants;
# # 	kn_opcode *opcodes;
# # } frame;


# # kn_frame *kn_parse(const char *_stream) {
# # 	stream = _stream;
# # 	frame.nlocals = 0;
# # 	frame.nconstants = 0;
# # 	frame.nopcodes = 0;

# # }

# # // typedef struct {
# # // 	unsigned nlocals, nconstants, nopcodes;

# # // 	kn_value *constants;
# # // 	kn_opcode *opcodes;
# # // } kn_frame;

# # // int main(){
# # // 	frame frame;
# # // }
