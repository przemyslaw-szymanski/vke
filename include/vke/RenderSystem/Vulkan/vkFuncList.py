import re
import os.path

def CheckExclude(name, excludes):
	for ex in excludes:
		if ex in name:
			return True
	return False
	
def WriteAllContains(entrySet, name, excludes, file):
	for entry in entrySet:
		if CheckExclude(entry, excludes):
			continue
		
		if name in entry:
			file.write("\n" + entry)

	return
	
def RemoveRangeFromText(strIn, strBegin, strEnd):
	begin = strIn.find(strBegin)
	end = strIn.find(strEnd)
	strOut = ""
	while begin >= 0:
		range = end - begin
		print begin
		print end
		print range
		strOut += strIn[begin:-range]
		begin = strIn.find(strBegin, end)
		end = strIn.find(strEnd, end)
		print strOut
	return

RemoveRangeFromText("#if 0 // Old WSI abc #endif // Old WSIadfsafs", "#if 0 // Old WSI", "#endif // Old WSI")
	
def ParseFiles(strInFiles, strOutFile, excludes):
	entrySet = set()
	pattern = re.compile("\*PFN_(\w+)")
	
	strHeader = "#ifndef VKE_VK_FUNCTION\n"
	strHeader += "#	define VKE_VK_FUNCTION(_name) extern PFN_##_name _name\n"
	strHeader += "#endif\n"
	
	outFile = open(strOutFile, "w")
	outFile.write(strHeader)
	
	for fileName in strInFiles:
		if not os.path.isfile(fileName):
			continue
		print("IN FILE: " + fileName)
		print("OUT FILE: " + strOutFile + "\n")
		
		inFile = open(fileName, "r")
		strFileData = inFile.read()
		iters = re.finditer(pattern, strFileData)
		inFile.close()
		
		for itr in iters:
			ptrName = itr.group(0).replace("*PFN_", "")
			if CheckExclude(ptrName, excludes):
				continue
				
			print(ptrName)
			entry = "VKE_VK_FUNCTION(" + ptrName + ");"
			entrySet.add(entry)
	
	for entry in entrySet:
		# write all not khr, android, linux, windows
		if CheckExclude(entry, ["WSI", "KHR", "Android", "Linux", "Windows"]):
			continue
		outFile.write("\n"+entry)
		
	# Write all Windows
	outFile.write("\n#if VKE_USE_VULKAN_WINDOWS")
	WriteAllContains(entrySet, "Win32", ["Android", "Linux", "Mir"], outFile)
	outFile.write("\n#endif // VKE_USE_VULKAN_WINDOWS")
	# Write all Linux
	outFile.write("\n#if VKE_USE_VULKAN_LINUX")
	WriteAllContains(entrySet, "Linux", ["Android", "Win32", "Mir"], outFile)
	outFile.write("\n#endif // VKE_USE_VULKAN_LINUX")
	# Write all Android
	outFile.write("\n#if VKE_USE_VULKAN_ANDROID")
	WriteAllContains(entrySet, "Android", ["Win32", "Linux", "Mir"], outFile)
	outFile.write("\n#endif // VKE_USE_VULKAN_ANDROID")
	# Write all WSI
	outFile.write("\n#if VKE_USE_VULKAN_WSI")
	WriteAllContains(entrySet, "WSI", ["Android", "Win32", "Linux", "Mir"], outFile)
	outFile.write("\n#endif // VKE_USE_VULKAN_WSI")
	# Write all KHR
	outFile.write("\n#if VKE_USE_VULKAN_KHR")
	WriteAllContains(entrySet, "KHR", ["Android", "Win32", "Linux", "Mir"], outFile)
	outFile.write("\n#endif // VKE_USE_VULKAN_KHR")
	
	
	outFile.write("\n#undef VKE_VK_FUNCTION\n")
	
	outFile.close()
	return

	
ParseFiles(["vulkan.h"], "vkFuncList.h", ["Function", "vkInternalFreeNotification", "vkInternalAllocationNotification"])
#ParseFiles(["vk_ext_khr_device_swapchain.h", "vk_ext_khr_swapchain.h"], "vkFuncListKHR.h", ["Function"])