"""Same-clock, original-size detail QA previews from completed quality_video results."""
import argparse
import html
import json
import subprocess
import tempfile
from pathlib import Path
from PIL import Image, ImageDraw

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--quality", type=Path, required=True)
    p.add_argument("--corpus", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    a=p.parse_args(); a.output.mkdir(parents=True, exist_ok=False)
    quality=json.loads((a.quality/"quality.json").read_text())
    corpus=json.loads((a.corpus/"manifest.json").read_text())
    previews=[]
    for record in quality["cases"]:
        name,case=record["clip"],record["case"]
        if name=="sidd":
            import shutil
            shutil.copy2(a.quality/name/case/"detail.png",a.output/"sidd-detail.png")
            continue
        source=next(c for c in corpus["clips"] if c["name"]==name)
        x,y,w,h=quality["fixed_rois"][name]
        sigma=15 if case=="gaussian15" else 5
        labels=["Reference", "Input", "Ordinary blur0.6", f"FFDNet sigma{sigma}", f"IRCNN sigma{sigma}", "NAFNet SIDD32"]
        methods=[None,"Input","Ordinary blur0.6","FFD weak-or-matched","IRCNN weak-or-matched","NAFNet SIDD32"]
        # One precomposed frame => every panel shows exactly the same frame/time.
        with tempfile.TemporaryDirectory(prefix="fstr-quality-preview-") as temporary:
            folder=Path(temporary)
            for i,frame in enumerate(source["frames"]):
                image=Image.new("RGB",(w*3,(h+28)*2+44),(18,18,20)); draw=ImageDraw.Draw(image)
                for j,method in enumerate(methods):
                    path=a.corpus/frame["file"] if method is None else a.quality/record["methods"][method]["display_folder"]/f"{i+1:03}.png"
                    tile=Image.open(path).convert("RGB").crop((x,y,x+w,y+h))
                    col,row=j%3*w,j//3*(h+28)
                    image.paste(tile,(col,row+28)); draw.text((col+8,row+8),labels[j],fill="white")
                draw.text((8,(h+28)*2+6),f"{name} / {case} / frame {i+1:02}/12 / 12fps / crop 1:1",fill="white")
                draw.text((8,(h+28)*2+23),"Tears of Steel - Blender Foundation / Blender Institute - CC BY 3.0 - Technical QA",fill=(160,160,164))
                image.save(folder/f"{i+1:03}.png")
            filename=f"{name}-{case}.mp4"
            subprocess.run(["ffmpeg","-v","error","-framerate","12","-i",str(folder/"%03d.png"),
                            "-c:v","libx264","-crf","10","-preset","medium","-threads","1",
                            "-vf","scale=in_range=pc:out_range=tv:out_color_matrix=bt709,format=yuv420p",
                            "-color_primaries","bt709","-color_trc","iec61966-2-1","-colorspace","bt709","-color_range","tv",
                            "-movflags","+faststart",str(a.output/filename)],check=True)
        previews.append({"clip":name,"case":case,"file":filename})
    options="".join(f'<option value="{html.escape(r["file"])}">{html.escape(r["clip"]+" / "+r["case"])}</option>' for r in previews)
    page='''<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width">
<title>FSTR-LUT — проверка качества</title><style>
body{margin:0;background:#161619;color:#eee;font:16px system-ui;padding:24px;max-width:1100px}h1{font-size:26px;font-weight:600}
p{max-width:980px;line-height:1.5}select,button{font:inherit;padding:8px;background:#303036;color:#fff;border:1px solid #62626e;border-radius:6px;margin:0 8px 12px 0}
video{display:block;width:960px;max-width:100%;background:#111}img{max-width:100%}.muted{color:#b1b1bc;font-size:14px}a{color:#bcccff}
</style><h1>FSTR-LUT: что действительно улучшает изображение</h1>
<p>Сравнение одинаковых кадров: исходник, повреждённый вход, обычное размытие и три модели. Все шесть панелей синхронны; фрагменты показаны без увеличения. Это короткая техническая проверка, выбор модели ещё открыт.</p>
<select aria-label="Сцена и дефект" id="choice">OPTIONS</select>
<button id="play">Смотреть в движении</button><button id="stop">Пауза</button>
<button id="reset">Первый кадр</button><button id="next">Следующий кадр</button><output id="state">Загрузка</output>
<video id="clip" muted loop playsinline preload="auto" src="FIRST"></video>
<p class="muted">Клипы 1 секунду при 12 кадрах/с, повтор по кругу. Между концом и началом есть искусственный скачок. Source — уже сжатый и обработанный фильм; его нельзя считать идеальным оригиналом камеры. Gaussian5/15 — добавленный шум, H264 — дополнительное сжатие, blur — добавленное размытие. Для source/H264/blur слабая настройка sigma5 задана заранее, автоматической оценки шума здесь нет.</p>
<p class="muted">В роликах сравнивается качество. Сохранённые численные оценки используют float-результат до обрезки диапазона и кодирования. Визуальные MP4 содержат одинаковое дополнительное кодирование CRF10. Эти примеры не подтверждают стабильность на длинном видео и работу внутри After Effects.</p>
<h2>Настоящий шум смартфона: официальный пример SIDD</h2><p class="muted">Один опубликованный пример, статическое изображение. NAFNet обучена на SIDD; это не независимый тест. <a href="https://abdokamel.github.io/sidd/">Источник и MIT-лицензия набора</a>.</p><img src="sidd-detail.png" alt="SIDD: одинаковые фрагменты настоящего шума и результатов моделей">
<p class="muted">Видео: <a href="https://mango.blender.org/about/">Tears of Steel</a>, Blender Foundation / Blender Institute, CC BY 3.0; техническая проверка. Фрагменты обрезаны и обработаны. Полные титры сохранены в локальном исходном фильме.</p>
<script>
const video=document.getElementById('clip'),choice=document.getElementById('choice'),state=document.getElementById('state');
function status(){state.value=(video.paused?'Пауза':'Идёт')+' · '+Math.min(12,Math.floor(video.currentTime*12)+1)+'/12'}
choice.onchange=()=>{video.pause();video.src=choice.value;video.load()};
document.getElementById('play').onclick=()=>video.play();document.getElementById('stop').onclick=()=>video.pause();
document.getElementById('reset').onclick=()=>{video.pause();video.currentTime=0};
document.getElementById('next').onclick=()=>{video.pause();video.currentTime=(Math.floor(video.currentTime*12)+1)%12/12+0.002};
['loadeddata','timeupdate','pause','play','seeked'].forEach(e=>video.addEventListener(e,status));setInterval(status,100);
</script></html>'''.replace("OPTIONS",options).replace("FIRST",previews[0]["file"])
    (a.output/"index.html").write_text(page)
    (a.output/"preview-manifest.json").write_text(json.dumps({"quality_report":"quality.json","quality_scope":quality["scope"],"previews":previews},indent=2)+"\n")
    print(json.dumps({"previews":len(previews),"output":str(a.output)}))
if __name__=="__main__":main()
