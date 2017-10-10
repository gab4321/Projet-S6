%% Initialisations %%

close all
clear all
clc

%% Spécifications à atteindre %%
%
% L'antenne doit être utilisable entre 2,4 GHz et 2,5 GHz
% L'antenne doit avoir une impédance de 50 ohm
%
%% Constantes %%

% Fréquence de transmission
f = 2.45e9;

% Vitesse de la lumiere
c = 3e8;

% Constante diélectrique du FR4
epsr = 4.36;

% facteur de conversion mils to m
mils_to_m = 254/(10e6);

%% Spécifications du substrat %%

% Épaisseur de 60 mils
h_mils = 60;
h = h_mils * mils_to_m;


% Espacement minimum entre deux traces
Esp_min_mils = 8;
Esp_min = Esp_min_mils * mils_to_m;

%% Calcul de W $$

W = c/(2*f)*sqrt(2/(epsr+1));

%% Calcul de la constante diélectrique effective %%

eps_eff = 0.5*(epsr+1) + 0.5 * (epsr-1) / (sqrt(1+12*h/W));

%% Calcul de la correction de la longueur %%
num_delta_L = (eps_eff + 0.300) * (W/h + 0.264);
den_delta_L = (eps_eff - 0.258) * (W/h + 0.800);
Delta_L = 0.412*h*num_delta_L/den_delta_L;

%% Calcul de la longueur physique %%

lambda_th = c/(sqrt(eps_eff)*f);
L_th = lambda_th/2;
L_ph = L_th-2*Delta_L;

%% Calcul de l'impédance de l'antenne %%

num_Za = 90*epsr*epsr*L_ph*L_ph;
den_Za = (epsr-1)*W*W;
Za = num_Za/den_Za














