#!/bin/env python3

import click
import tekore as tk
from requests import session
from requests.models import Response


def send_message(message: str, style: int=0, repeat: int=3, colour: str="#FF0000", fastness: int=70):
    data = {
        'message': message,
        'colorstyle': style,
        'repeat': repeat,
        'colour': colour,
        'speed': fastness
    }
    response = session().get("http://pixelsign.home.lan/text", params=data)


@click.group()
def pixelsign():
    pass

@click.command()
@click.argument('message', default='Hail Hydra!')
@click.option('-s', '--style', default="0")
@click.option('-r', '--repeat', default="3")
@click.option('-c', '--colour', default="#FF0000")
@click.option('-f', '--fastness', default=70)
def message(message: str, style: int, repeat: int, colour: str, fastness: int):
    send_message(**locals())

@click.command()
def spotify():
    client_id = "<spotifyClientId>"
    client_secret = "<spotifyClientSecret>"
    redirect_uri = "http://localhost"

    conf = (client_id, client_secret, redirect_uri)
    file = 'tekore.cfg'

    try:
        conf = tk.config_from_file(file, return_refresh=True)
        token = tk.refresh_user_token(*conf[:2], conf[3])
    except FileNotFoundError:
        token = tk.prompt_for_user_token(*conf, scope=tk.scope.every)
        tk.config_to_file(file, conf + (token.refresh_token,))
    spot = tk.Spotify(token)
    current_track = spot.playback_currently_playing()
    if current_track and current_track.is_playing:
        name = current_track.item.name
        artist = " + ".join([a.name for a in current_track.item.artists])
        display_message = f"{name} - {artist}"
        print(display_message)
        send_message(message=display_message, style=2, repeat=2)
    else:
        print("Nothing Playing")

pixelsign.add_command(message)
pixelsign.add_command(spotify)

if __name__ == "__main__":
    pixelsign()