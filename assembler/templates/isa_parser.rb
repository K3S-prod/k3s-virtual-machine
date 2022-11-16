require 'yaml'
require 'erb'

class ISA
    def self.init()
        # TODO: implement proper struct layout.
        # raw yaml format shouldn't be used but instead parsed to
        # a struct to maintain file format structure.
        @yaml = YAML.load_file(ARGV[0])  
        @opcode_groups = @yaml["opcodes"]["groups"]
        @opcode_signatures = @yaml["signatures"]   
    end
    def self.opcode_groups
        @opcode_groups
    end
    def self.opcode_signatures
        @opcode_signatures
    end
    def self.tokenize_signature(signature)
        args = {}
        args["types"] = []
        args["sizes"] = []
        signature.split('_').each_with_index do |operand, idx|
            if idx == 0
                next
            end
            type_char = operand.slice!(0)
            if type_char == "r" then
                args["types"].append "REG"
            elsif type_char == "i" then
                args["types"].append "IMM"
            end
            args["sizes"].append(operand)
        end
        args
    end
    def self.numbers(num)
        ["$1", "$2", "$3", "$4"].slice(0, num)
    end
end

ISA.init

n_templates = 3
(1..n_templates).each do |i|
    template_erb = ARGV[i]
    template_instance = ARGV[i + n_templates]
    template = ERB.new File.read(template_erb), nil, "-"
    File.open(template_instance, 'w') { |file| file.write(template.result) }
end
