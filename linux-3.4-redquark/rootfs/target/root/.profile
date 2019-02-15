stty cols 132 rows 34
alias lsusb='/usr/bin/lsusb|awk '\''BEGIN{ie="[[:xdigit:]]{4}";ve="^"ie"[[:space:]]+";pe="^\t"ie"[[:space:]]+";while(("cat /usr/share/misc/usb.ids")|getline)if($0=="# List of known device classes, subclasses and protocols"){break}else if(lv==""&&substr($1,0,1)=="#"){continue}else if(lv!=""&&$0~pe){lp=$1;sub(pe,"");r[lv,lp]=$0}else if($0~ve){lv=$1;sub(ve,"");r[lv]=$0}}"ID "ie":"ie"$"{split($6,id,":");printf "%s %s %s\n",$0,r[id[1]],r[id[1],id[2]]}'\'

# for setting history length
export HISTSIZE=1000
export HISTFILESIZE=2000

# Prompt Settings
export PS1="\\u@\\h (`ifconfig | grep 'inet addr:'| grep -v '127.0.0.1' | tail -1 | cut -d: -f2 | awk '{ print $1}'`):\\w\\$"

# This sets nano as the default editor
export EDITOR="/usr/bin/nano -c -z"
# Prevent nano from wrapping long lines
alias nano='/usr/bin/nano -c -z'
cat /etc/issue

