#!/bin/bash
#$1 root_directory
#$2 text_file
#$3 w = number of web sites
#$4 p = number of web pages per web site

# ./webcreator.sh root_directory text_file w p
# ./webcreator.sh root_directory text_file 2 3
#echo $1 $2 $3 $4
rootdir=$1
textfile=$2
nosites=$3
nopages=$4
echo $rootdir $textfile $nosites $nopages
if [ -e $textfile ];then #if textfile exists
    if (( [[ $nosites =~ ^-?[0-9]+$ ]] && [[ $nosites -gt 0 ]] ) && ( [[ $nopages =~ ^-?[0-9]+$ ]] && [[ $nopages -gt 0 ]] ));then #number of sites, pages are valid integers
        lines=`wc -l $textfile | cut -f1 -d' '` #count lines
        echo "lines = $lines"
        if [ $lines -ge 10000 ]
        then
            echo "This file has more than 10000 lines."
            if [ -d "$rootdir" ]; then
                if [ -n "$(ls -A $rootdir)" ];then
                    echo "Warning: directory is full, purging …"
                    rm -r $rootdir
                    mkdir $rootdir
                fi
            else
                mkdir $rootdir
            fi
            for i in $(seq 0 $(($nosites-1)));do 
                sitename="/site"
                sitename="$sitename$i"
                mkdir "$rootdir$sitename"
                rand=($(shuf -e $(seq 1 10000)))
                for j in $(seq 1 $nopages);do
                    pagename=("$sitename/page$i""_${rand[$j]}.html ")
                    # echo $pagename
                    pagenames+=($pagename)
                done
            done
            countlinks=()
            for ((i=0; i < ${#pagenames[@]}; i++))
            do
                countlinks+=(0)
            done
            flag=0
            for ((i=0; i < ${#pagenames[@]}; i++))
            do
                k=$(( ( RANDOM % ($lines-2000) )  + 2 )) #1<k<lines-2000
                m=$(( ( RANDOM % 1000 )  + 1000 )) #1000<m<2000

                echo "Creating web site $(($i/$nopages))"
                echo "  Creating page $rootdir${pagenames[$i]} with $m lines starting at line $k"
                #internal links
                selectedinternalpages=()
                indexstart=$((($i/$nopages)*$nopages))
                for ((j=$indexstart; j < $(($indexstart+$nopages)); j++))
                do
                    if [ $j -ne $i ] || [ $nopages -le 2 ];then
                        selectedinternalpages+=($j)
                    fi
                done
                selectedinternalpages=( $(shuf -e "${selectedinternalpages[@]}") )
                internalpages=()
                for ((j=0; j < $((($nopages/2)+1)); j++))
                do
                    internalpages+=(${pagenames[${selectedinternalpages[$j]}]})
                done

                #external links
                startofsite=$(((($i/$nopages)*$nopages)))
                selectedexternalpages=()
                for ((j=0; j < ${#pagenames[@]}; j++))
                do
                    if [ $((($j/$nopages)*$nopages)) -eq $startofsite ];then
                        j=$(($j+$nopages-1))
                    else
                        selectedexternalpages+=(${pagenames[$j]})
                    fi
                done
                selectedexternalpages=( $(shuf -e "${selectedexternalpages[@]}") )
                externalpages=()
                for ((j=0; j < $((($nosites/2)+1)); j++))
                do
                    externalpages+=(${selectedexternalpages[$j]})
                done

                #write html to file
                totallinks=("${internalpages[@]}" "${externalpages[@]}")
                totallinks=( $(shuf -e "${totallinks[@]}") )
                echo -e "<!DOCTYPE html>\n<html>\n\t<body>" >> "$rootdir${pagenames[$i]}"
                linecount=0
                copied=0
                nolink=0
                while read line; do
                    if [ $linecount -ge $k ] && [ $copied -lt $(($m/(${#internalpages[@]}+${#externalpages[@]}))) ];then
                        echo $line >> "$rootdir${pagenames[$i]}"
                        copied=$(($copied+1))
                    elif [ $copied -ge $(($m/(${#internalpages[@]}+${#externalpages[@]}))) ];then
                        copied=0
                        echo "  Adding link to $rootdir${totallinks[$nolink]}"
                        for ((j=0; j < ${#pagenames[@]}; j++))
                        do
                            if [ ${pagenames[$j]} = ${totallinks[$nolink]} ];then
                                countlinks[$j]=$((${countlinks[$j]}+1))
                            fi
                        done
                        echo -e "<a href="${totallinks[$nolink]}">${totallinks[$nolink]}</a>" >> "$rootdir${pagenames[$i]}"
                        nolink=$(($nolink+1)) 
                        if [ $nolink -ge ${#totallinks[@]} ];then
                            break
                        fi
                    fi
                    linecount=$(($linecount+1))                    
                done < "$textfile"
                echo -e "\t</body>\n</html>" >> $rootdir${pagenames[$i]}
            done
            for ((j=0; j < ${#countlinks[@]}; j++))
            do
                if [ ${countlinks[$j]} -eq 0 ];then
                    flag=1
                fi
            done
            echo "Done."
            if [ $flag -eq 0 ];then
                echo "All pages have at least one incoming link"
            fi
        fi
    else
        echo "numbers are not intergers or positive"
    fi
else
    echo "textfile doesn't exist"
fi