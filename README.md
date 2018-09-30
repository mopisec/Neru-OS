Neru OS
===

About Neru OS
---------------
Neru OS is hobby OS developed by moppoi5168.<br>
The base part of OS is HariboteOS that is developed in "30日でできる！OS自作入門".

How to build / run
---------------
Makefile use qemu-system-i386 for running OS.<br>
Please prepare tolset tools for your environment.<br>
(Don't forget to rename tolset folder to 'Tools', or just change Makefile)<br>
Type and excute 'make full' to build OS.<br>
('make full' also create iso image of Neru OS)<br>
Type and excute 'make run' to run OS using QEMU.<br>
For other option, please look at Makefile or "30日でできる！OS自作入門".


Features
---------------
* Original Window Design
* Serial Communication (can be used for debug)
* PCI Bus Configuration (Now on the Development: Just available to output PCI Vendor Name)
* NIC Driver (RTL8139, Not implemented yet)

License
---------------
This software is licensed by MIT License.
