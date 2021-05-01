#!/bin/ruby

$pattern = ["*.cpp", "*.c"]

def getDependFile(f)
    r = Array.new

    IO.foreach(f) do |line|
        if (line =~ /\#include\s*\"/) then
            part = line.split('"');
            r |= [part[1]]
            tmp = getDependFile(part[1])
            if tmp != nil then
                r |= tmp
            end
        end
    end

    return r
end

def getObject(f)
    part = f.split('.')
    sfx = part.last
    return File.basename(f, sfx) + "obj"
end

obj = Array.new
    
$pattern.each do |pat|

    Dir.glob(pat) do |f|
        a = getDependFile(f)
        obj.push(getObject(f))
        printf "%s: ", getObject(f)
        printf "%s ", f
        a.each do |dep|
            printf "%s ", dep
        end
        printf "\n"
        printf "\t%s\n", '$(CC) $(CFLAG) ' + f
        printf "\n"
    end

end

Dir.glob("*.asm") do |f|
    obj.push(getObject(f))
    printf "%s: ", getObject(f)
    printf "%s\n", f
    printf "\t%s\n\n", '$(ASM) $(AFLAG) ' + f
end

printf "OBJ = "
obj.each do |f|
    printf "%s ", f
end
printf "\n\n"

printf "AOBJ: "
Dir.glob("*.asm") do |f|
    printf "%s ", getObject(f)
end
printf "\n\n"