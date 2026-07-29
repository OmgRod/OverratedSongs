import csv
import json
import os

input_file = 'songs.csv'
output_json = 'res/files/songs.json'

songs_data = []

with open(input_file, 'r', encoding='utf-8') as f_in:
    reader = csv.reader(f_in)
    
    next(reader, None)
    
    for row in reader:
        if len(row) >= 12:
            try:
                song_id = int(row[0])
                frequency = int(row[11])
                
                if frequency > 0:
                    songs_data.append({
                        "songID": song_id,
                        "frequency": frequency
                    })
            except ValueError:
                continue

with open(output_json, 'w', encoding='utf-8') as f_out:
    json.dump(songs_data, f_out, indent=4)

print(f"Data converted to JSON and saved to {output_json}")
