import re
import os.path

def ParseFiles(strInFiles, strOutFile1, strOutFile2):
	entrySet = set()
	pattern = re.compile("VK_FORMAT_(\w+)")
	
	outFile1 = open(strOutFile1, "w")
	outFile1.write("#pragma once\n")
	outFile1.write("namespace VKE\n{\n")
	outFile1.write("\tnamespace RenderSystem\n\t{\n")
	outFile2 = open(strOutFile2, "w")
	outFile2.write("#pragma once\n")
	outFile2.write("namespace VKE\n{\n")
	outFile2.write("\tnamespace RenderSystem\n\t{\n")
	
	vkFormats = []
	vkeFormats = []
	
	for fileName in strInFiles:
		if not os.path.isfile(fileName):
			continue
		inFile = open(fileName, "r")
		strFileData = inFile.read()	
		iters = re.finditer(pattern, strFileData)
		inFile.close()
		
		for itr in iters:
			s = itr.group(0)
			#print(str + "\n")
			if "FEATURE" in s:
				continue
			if "BEGIN_RANGE" in s:
				break
				
			vkeName = s.replace("VK_FORMAT_", "")
			vkFormats.append(s)
			vkeFormats.append(vkeName)
	
	strFormatStruct = ""
	strFormatStruct += "\t\tstruct ImageFormats\n\t\t{\n"
	strFormatStruct += "\t\t\tenum FORMAT\n\t\t\t{\n"
	fmtCount = 0
	for fmt in vkeFormats:
		strFormatStruct += "\t\t\t\t" + fmt + ",\n"
		fmtCount += 1
	strFormatStruct += "\t\t\t\t" + "_MAX_COUNT\n"
	strFormatStruct += "\t\t\t};\n"
	strFormatStruct += "\t\t};\n"
	strFormatStruct += "\t\tusing IMAGE_FORMAT = ImageFormats::FORMAT;"
	
	outFile2.write(strFormatStruct)
	
	strFormatArray = ""
	strFormatArray += "\n\t\t" + "static const VkFormat g_aFormats[VK_FORMAT_END_RANGE+1] ="
	strFormatArray += "\n\t\t" + "{"
	fmtCount = 0
	for fmt in vkFormats:
		strFormatArray += "\n\t\t\t" + fmt + ","
		fmtCount += 1
	# remove last character ,
	strFormatArray = strFormatArray[:-1]
	strFormatArray += "\n\t\t};"
	
	outFile1.write(strFormatArray)
	
	outFile1.write("\n\t} // RenderSystem")
	outFile1.write("\n} // VKE")
	outFile2.write("\n\t} // RenderSystem")
	outFile2.write("\n} // VKE")
	
	outFile1.close()
	outFile2.close()
	return

	
ParseFiles(["vulkan.h"], "vkFormatList.h", "VKEImageFormats.h")
