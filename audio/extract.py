import vpk, yaml, pydub, os, sys

pak = vpk.open(sys.argv[1])
if len(sys.argv) == 3:
    pak_fallback = vpk.open(sys.argv[2])
def get_pak(path):
    if path in pak:
        return pak.get_file(path)
    else:
        return pak_fallback.get_file(path)

sounds = yaml.safe_load(open('sounds.yml'))

out = []
for dir in sounds.keys():
    if not os.path.exists(f"out/{dir}"):
        os.mkdir(f"out/{dir}")
    i = 0
    for files in sounds[dir]:
        files = files.split(":")
        i += 1
        print(f"{dir}/{i:03}.mp3 ({files})")
        open("temp.wav", "wb").write(get_pak(files[0]).read())
        sound = pydub.AudioSegment.from_wav("temp.wav")
        for file in files[1:]:
            open("temp.wav", "wb").write(get_pak(file.split("@")[0]).read())
            sound = sound.append(pydub.AudioSegment.from_wav("temp.wav"), crossfade=int(file.split("@")[1]))
        sound.export(f"out/{dir}/{i:03}.mp3", format="mp3", parameters=["-ac", "1", "-b:a", "40k"])
    out.append(str(len(sounds[dir])))
open("../generated/audio.h", "w").write(f"uint8_t sounds_number[{len(sounds)}] = {{" + ",".join(out) + "};")