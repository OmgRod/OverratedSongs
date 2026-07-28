import csv
import json
import os

input_file = 'songs.csv'
output_json = 'res/files/songs.json'

# The goal is a list of objects like: {"songID": 1, "frequency": 15}
# Based on the CSV header:
# Song ID (index 0), ..., Total Rated Levels (index 11)

songs_data = []

with open(input_file, 'r', encoding='utf-8') as f_in:
    reader = csv.reader(f_in)
    
    # Skip original header
    next(reader, None)
    
    for row in reader:
        if len(row) >= 12: # Ensure row has at least 12 columns
            try:
                song_id = int(row[0])
                frequency = int(row[11])
                
                # Only add if weight/frequency is valid
                if frequency > 0:
                    songs_data.append({
                        "songID": song_id,
                        "frequency": frequency
                    })
            except ValueError:
                # Skip rows with invalid integer values
                continue

# Write to JSON
with open(output_json, 'w', encoding='utf-8') as f_out:
    json.dump(songs_data, f_out, indent=4)

print(f"Data converted to JSON and saved to {output_json}")
