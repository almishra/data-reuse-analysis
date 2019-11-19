#cd /gpfs/projects/ChapmanGroup/alok/src/data-reuse/data-reuse-analysis/DataReuse
DIR=/gpfs/projects/ChapmanGroup/alok/src/data-reuse/data-reuse-analysis/DataReuse
lib="$DIR/.build/src/kernel-info/libkernel-info.so"
plugin="-kernel-info"
while [ "$1" != "" ]; do
    case $1 in
        -d | --data )
            shift;
            lib="$DIR/.build/src/data-analysis/libdata-analysis.so"
            plugin="-data-analysis"
            break;
            ;;

        -l | --loop)
            shift;
            lib="$DIR/.build/src/loop/libfind-loop.so"
            plugin="-find-loop"
            break;
            ;;

        -k | --kernel)
            shift;
            lib="$DIR/.build/src/kernel/libfind-kernel.so"
            plugin="-find-kernel"
            break;
            ;;

        -p | --proximity)
            shift;
            lib="$DIR/.build/src/proximity/libfind-near-kernel.so"
            plugin="-find-near-kernel"
            break;
            ;;

        -ki | --kerninfo )
            shift;
            lib="$DIR/.build/src/kernel-info/libkernel-info.so"
            plugin="-kernel-info"
            break;
            ;;
        -dr | --datareuse)
            shift;
            lib="$DIR/.build/src/data-reuse/libdata-reuse.so"
            plugin="-data-reuse"
            break;
            ;;
        * )
            break;
            ;;
    esac
done

filename=$1;
if [ "$filename" == "" ]
then
    filename=$DIR/example/test.c
fi

echo "clang -Xclang -load -Xclang $lib -Xclang -plugin -Xclang $plugin -fopenmp -lm -c $filename"
echo
clang -Xclang -load -Xclang $lib -Xclang -plugin -Xclang $plugin -fopenmp -c $filename
