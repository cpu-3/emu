class Disasm
{
    std::string reg(uint32_t num){
        if(num == 0){
            return "x0";
        }
        else if (num == 1){
            return "x1";
        }
        else if (num == 2){
            return "sp";
        }
        else if (num == 3){
            return "gp";
        }
        else if (num == 4){
            return "hp";
        }
        else if (num >= 5 && num <= 7){
            return "t" + std::to_string(num-5);
        }
        else if (num == 8){
            return "fp";
        }
        else if (num == 9){
            return "s1";
        }
        else if (num >= 10 && num <= 17){
            return "a" + std::to_string(num-10);
        }
        else if (num >= 18 && num <= 27){
            return "s" + std::to_string(num-16);
        }
        else if (num >= 28 && num <= 31){
            return "t" + std::to_string(num-25);
        }
    }
    std::string freg(uint32_t num){
        return "f" + std::to_string(num);
    }


  public:
    std::string type;
    std::string inst_name;
    int32_t imm;
    uint32_t dest;
    uint32_t src;
    uint32_t src1;
    uint32_t src2;
    uint32_t base;

    void print_inst(std::string type){
        if(type == "r"){
            std::cout << inst_name << ", "
                      << reg(dest) << ","
                      << reg(src1) << ","
                      << reg(src2) << std::endl;
        }
        else if(type == "i"){
            std::cout << inst_name << ", "
                      << reg(dest) << ","
                      << reg(base) << ","
                      << imm << std::endl;
        }
        else if(type == "s"){
            std::cout << inst_name << ", "
                      << reg(src) << ","
                      << reg(base) << ","
                      << imm << std::endl;
        }
        else if(type == "b"){
            std::cout << inst_name << ", "
                      << reg(src1) << ","
                      << reg(src2) << ","
                      << imm << std::endl;
        }
        else if(type == "u" || type == "j"){
            std::cout << inst_name << ", "
                      << reg(dest) << ","
                      << imm << std::endl;
        }
        else if(type == "fr"){
            std::cout << inst_name << ", "
                      << freg(dest) << ","
                      << freg(src1) << ","
                      << freg(src2) << std::endl;
        }
        else if(type == "fi"){
            std::cout << inst_name << ", "
                      << freg(dest) << ","
                      << reg(base) << ","
                      << imm << std::endl;
        }
        else if(type == "fs"){
            std::cout << inst_name << ", "
                      << freg(src) << ","
                      << reg(base) << ","
                      << imm << std::endl;
        }
        else if(type == "fb"){
            std::cout << inst_name << ", "
                      << freg(src1) << ","
                      << freg(src2) << ","
                      << imm << std::endl;
        }
        else if(type == "fu" || type == "fj"){
            std::cout << inst_name << ", "
                      << freg(dest) << ","
                      << imm << std::endl;
        }
        else if(type == "fR"){
            std::cout << inst_name << ", "
                      << freg(dest) << ","
                      << freg(src1) << std::endl;
        }
    }
};

