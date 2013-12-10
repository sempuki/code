all: install-packages copy-configuration

install-packages: update-packages basic-packages development-packages desktop-packages font-packages media-packages application-packages

update-packages:
	sudo yum -y update

basic-packages:
	sudo yum -y install vim-X11 vim-enhanced

development-packages:
	sudo yum -y install gcc-c++ nemiver 

application-packages:
	sudo yum -y install pidgin pidgin-otr

desktop-packages:
	sudo yum -y install gnome-tweak-tool dconf-editor

font-packages:
	sudo yum -y install fonts-tweak-tool freetype-freeworld

media-packages:
	sudo yum -y install gstreamer-ffmpeg gstreamer-plugins-ugly

copy-configuration:
	cp vimrc ~/.vimrc
	cp bashrc ~/.bashrc
	cp gitconfig ~/.gitconfig
	cp gitignore ~/.gitignore
	source ~/.bashrc
