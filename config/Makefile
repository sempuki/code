all: install-packages install-scripts copy-config

install-packages: update-packages basic-packages development-packages desktop-packages font-packages media-packages application-packages

update-packages:
	sudo dnf -y update

development-packages:
	sudo dnf -y install vim-X11 vim-enhanced
	sudo dnf -y install gcc-c++ gdb git global-ctags
	rm -rf ~/.vim/bundle/Vundle.vim && mkdir -m 755 -p ~/.vim/bundle
	git clone https://github.com/gmarik/Vundle.vim.git ~/.vim/bundle/Vundle.vim

desktop-packages:
	sudo dnf -y install redshift-gtk gnome-tweak-tool dconf-editor

# requires rpmfusion.org repo
font-packages:
	sudo dnf -y install fonts-tweak-tool freetype-freeworld

# requires rpmfusion.org repo
media-packages:
	sudo dnf -y install gstreamer-ffmpeg gstreamer-plugins-ugly

install-scripts:
	mkdir ~/Scripts && cp ../scripts/* ~/Scripts

bash-config:
	cp bashrc ~/.bashrc
	source ~/.bashrc

git-config:
	cp gitconfig ~/.gitconfig
	cp gitignore ~/.gitignore

vim-config: install-scripts
	cp vimrc ~/.vimrc

vimperator-config:
	cp vimperatorrc ~/.vimperatorrc

neovim-config:
	ln -s ~/.vim ~/.config/nvim 
	ln -s ~/.vimrc ~/.config/nvim/init.vim

ctags-config:
	cp ctags ~/.ctags

ack-config:
	cp ackrc ~/.ackrc

copy: bash-config git-config vim-config vimperator-config ctags-config ack-config
