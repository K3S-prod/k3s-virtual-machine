require 'yaml'
require 'erb'

def ASSERT(cond)
    raise RuntimeError unless cond
end

class ISA
    def self.init()
        # TODO: implement proper struct layout.
        # raw yaml format shouldn't be used but instead parsed to
        # a struct to maintain file format structure.
        @yaml = YAML.load_file(ARGV[0])  
        @opcode_signatures = @yaml["signatures"]   
        @opcode_groups = @yaml["opcodes"]["groups"]
        @opcode_overload_limit = @yaml["opcodes"]["opcode_overload_limit"].to_i
        @reg_types = @yaml["reg_types"]
    end
    def self.opcode_groups
        @opcode_groups
    end
    def self.opcode_signatures
        @opcode_signatures
    end
    def self.opcode_overload_limit
        @opcode_overload_limit
    end
    def self.reg_types
        @reg_types
    end
    def self.GetDispatchTable()
        disaptch_table = []
        temp_label_str = ""
        opcode_groups.each  do |key, subgroups| 
            key
            subgroups.each do |subgroup|
            subgroup["opc"].each do |opcode|
                subgroup["overloads"].each do |overload|
                    temp_label_str = GetOverloadLabel(opcode, overload)
                    disaptch_table.append(temp_label_str)
                end
                # fill empty overloads with nullptr:

                (subgroup["overloads"].length .. (opcode_overload_limit - 1)).each do
                    disaptch_table.append("nullptr")
                end
            end
        end end
        return disaptch_table
    end

    def self.GetOverloadLabel(opcode, overload)
        ASSERT(opcode.is_a? String)
        label = opcode.upcase
        overload["in"].each do |arg|
            label += '_' + ParseOverloadArg(arg).join("")
        end
        return label
    end

    def self.ParseOverloadArg(arg)
        splitted = arg.split(':')
        ASSERT(splitted.length == 2)
        return splitted
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
    def self.GetGrammarArgs(num)
        ["$1", "$2", "$3", "$4"].slice(0, num)
    end
end

ISA.init

#TODO: handle arguments properly
n_templates = ARGV.length / 2
(1..n_templates).each do |i|
    template_erb = ARGV[i]
    template_instance = ARGV[i + n_templates]
    template = ERB.new File.read(template_erb), nil, "-"
    File.open(template_instance, 'w') { |file| file.write(template.result) }
end
