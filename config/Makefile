all: install-packages install-scripts copy-configuration

install-packages: update-packages basic-packages development-packages desktop-packages font-packages media-packages application-packages internet-packages

update-packages:
	sudo dnf -y update

basic-packages:
	sudo dnf -y install vim-X11 vim-enhanced

development-packages:
	sudo dnf -y install gcc-c++ git nemiver global

application-packages:
	sudo dnf -y install pidgin pidgin-otr

desktop-packages:
	sudo dnf -y install redshift-gtk gnome-tweak-tool dconf-editor

# requires rpmfusion.org repo
font-packages:
	sudo dnf -y install fonts-tweak-tool freetype-freeworld

# requires rpmfusion.org repo
media-packages:
	sudo dnf -y install gstreamer-ffmpeg gstreamer-plugins-ugly

# requires adobe and google-chrome repo
internet-packages:
	sudo dnf -y install flash-plugin \
		qtwebkit.i686 libXScrnSaver.i686 && \
		sudo sed -i 's/Exec=skype/Exec=env PULSE_LATENCY_MSEC=30 skype/' \
		/usr/share/applications/skype.desktop

install-scripts:
	mkdir ~/Scripts && cp ../scripts/* ~/Scripts

bash-configuration:
	cp bashrc ~/.bashrc
	source ~/.bashrc

git-configuration:
	cp gitconfig ~/.gitconfig
	cp gitignore ~/.gitignore

vim-configuration: basic-packages development-packages
	cp vimrc ~/.vimrc
	mkdir -m 755 -p ~/.vim/bundle
	git clone https://github.com/gmarik/Vundle.vim.git ~/.vim/bundle/Vundle.vim

mutt-configuration:
	mkdir -m 700 -p ~/.mutt/cache/headers
	mkdir -m 700 -p ~/.mutt/cache/bodies
	mkdir -m 700 -p ~/.mutt/cache/temp
	mkdir -m 700 -p ~/.mutt/certificates
	mkdir -m 700 -p ~/.mutt/mailcap
	cp muttrc ~/.muttrc
	cp mailcap ~/.mutt

copy-configuration: bash-configuration git-configuration vim-configuration
