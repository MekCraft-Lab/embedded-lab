# 1. 获取脚本自身所在绝对路径
set script_work_path [file dirname [file normalize [info script]]]
set target_xsa ""
set bsp_out_dir [file normalize "$script_work_path/standalone_bsp"]

# 2. 检查是否通过命令行传入xsa路径

if {$argc > 0} {
	
	if {$argc == 1} {
		set input_path [lindex $argv 0]
		set ext [string tolower [file extension $input_path]]


		if {$ext eq ""} {
			set input_path "${input_path}.xsa"
			set ext ".xsa"
		}	
		
		if {[file exists $input_path]} {
			if {$ext eq ".xsa"} {
				set target_xsa $input_path
			} else {
				puts "\[ERR\]: Format error"
			}
		} else {
			puts "\[ERR\]: File name error"
		}
		
	} else {
		puts "\[ERR\]: Parameters error"
	}
}

# 3. 没有传入参数自动在预设路径中搜索.xsa文件

if {$target_xsa eq ""} {
	set found_files [list]
	set matched [glob -nocomplain -directory ${script_work_path} -types f "*.xsa"]
	
	foreach f $matched {
		lappend found_files [file normalize $f]
	}
	
	if {[llength $found_files] == 0} {
		puts "\[ERR\]: No .xsa file"
		exit 1
	} elseif {[llength $found_files] != 1} {
		puts "\[ERR\]: Too many .xsa files"
		exit 1
	} else {
		set target_xsa [lindex $found_files 0]
	}
}

puts "\n\[INFO\]: Found .xsa file \[${target_xsa}\]\n"
puts "\[INFO\]: BSP output path \[${bsp_out_dir}\]\n"

# 4. 调用HSI打开硬件设计并生成/编译BSP
hsi::open_hw_design $target_xsa
hsi::create_sw_design "standalone_bsp" -os "standalone" -proc "ps7_cortexa9_0"
hsi::generate_bsp -dir $bsp_out_dir -compile
hsi::close_hw_design [hsi::current_hw_design]

puts "\n\[SUCCESS\]: Standalone BSP generate completed!\n"
exit 0
