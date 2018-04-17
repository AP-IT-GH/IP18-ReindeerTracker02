import { Component, ViewChild, ElementRef } from '@angular/core';
import { IonicPage, NavController, NavParams } from 'ionic-angular';
import { LaunchNavigator} from '@ionic-native/launch-navigator'

/**
 * Generated class for the MapPage page.
 *
 * See https://ionicframework.com/docs/components/#navigation for more info on
 * Ionic pages and navigation.
 */

declare var google: any;

@IonicPage()
@Component({
  selector: 'page-map',
  templateUrl: 'map.html',
})
export class MapPage {

  @ViewChild('map') mapRef: ElementRef;

  map: any;

  constructor(public navCtrl: NavController, public navParams: NavParams, private launchNavigator: LaunchNavigator) {
  }

  ionViewDidLoad() {
    this.showMap();

    this.addRadius(new google.maps.LatLng(51.058518, 5.271671), this.map);
  }

  showMap(){
    //location - lat long
    const location = new google.maps.LatLng(51.058518, 5.271671);

    //map options
    const options = {
      center: location,
      zoom : 13,
      streetViewControl: false,
      mapTypeId: 'terrain'
    }

    this.map = new google.maps.Map(this.mapRef.nativeElement, options);
  }

  addRadius(position, map){
    return new google.maps.Circle({
      strokeColor: '#FF0000',
      strokeOpacity: 0.8,
      strokeWeight: 2,
      fillColor: '#FF0000',
      fillOpacity: 0.35,
      map: map,
      center: position,
      radius: 1000
    });
  }

  navMe(address){
    this.launchNavigator.navigate(address);
  }

}
