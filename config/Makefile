all: install-packages install-scripts copy-configuration

install-packages: update-packages basic-packages development-packages desktop-packages font-packages media-packages application-packages internet-packages

update-packages:
	sudo yum -y update

basic-packages:
	sudo yum -y install vim-X11 vim-enhanced

development-packages:
	sudo yum -y install gcc-c++ nemiver global

application-packages:
	sudo yum -y install pidgin pidgin-otr

desktop-packages:
	sudo yum -y install redshift-gtk gnome-tweak-tool dconf-editor

# requires rpmfusion.org repo
font-packages:
	sudo yum -y install fonts-tweak-tool freetype-freeworld

# requires rpmfusion.org repo
media-packages:
	sudo yum -y install gstreamer-ffmpeg gstreamer-plugins-ugly

# requires adobe and google-chrome repo
internet-packages:
	sudo yum -y install flash-plugin google-chrome-stable \
		qtwebkit.i686 libXScrnSaver.i686 && \
		sudo sed -i 's/Exec=skype/Exec=env PULSE_LATENCY_MSEC=30 skype/' \
		/usr/share/applications/skype.desktop

install-scripts:
	mkdir ~/Scripts && cp ../scripts/* ~/Scripts

copy-configuration:
	cp vimrc ~/.vimrc
	cp bashrc ~/.bashrc
	cp gitconfig ~/.gitconfig
	cp gitignore ~/.gitignore
	source ~/.bashrc
